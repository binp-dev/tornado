import os
import time
import zmq

from threading import Thread

from utils.epics.ioc import Ioc
from utils.epics import ca
import utils.proto as proto

IDS = None

def load_uint24(data):
    return data[0] | (int(data[1]) << 8) | (int(data[2]) << 16)

def store_uint24(value):
    assert value >= 0 and value < (1 << 24)
    return bytes([value & 0xff, (value >> 8) & 0xff, (value >> 16) & 0xff])

def encode(id, data=b""):
    assert(len(data) <= 256 - 1)
    return bytes([id]) + data

def encode_adc_val(index, value):
    return encode(IDS["IPP_MCU_ADC_VAL"], bytes([index]) + store_uint24(value))

def decode_dac_set(data):
    assert len(data) == 4
    assert int(data[0]) == IDS["IPP_APP_DAC_SET"]
    return load_uint24(data[1:])

def decode_adc_req(data):
    assert len(data) == 2
    assert int(data[0]) == IDS["IPP_APP_ADC_REQ"]
    return int(data[1])

def assert_eq(a, b, eps=1e2):
    assert abs(a - b) < eps

def run_test(
    epics_base_dir,
    ioc_dir,
    common_dir,
    arch,
):
    global IDS

    ioc = Ioc(
        os.path.join(ioc_dir, "bin", arch, "PSC"),
        os.path.join(ioc_dir, "iocBoot/iocPSC/st.cmd")
    )

    context = zmq.Context()
    socket = context.socket(zmq.PAIR)
    socket.bind("tcp://127.0.0.1:8321")

    prefix = os.path.join(epics_base_dir, "bin", arch)
    IDS = proto.read_defines(os.path.join(common_dir, "proto.h"))
    def id_as_bytes(id):
        return bytes([id])

    global done
    global value
    done = False
    value = 0

    max_val = 0xFFFFFF
    some_val = 0xABCDEF

    def worker():
        global done
        global value

        start_msg = socket.recv()
        assert(int(start_msg[0]) == IDS["IPP_APP_START"])
        print("Received start signal")
        socket.send(encode(IDS["IPP_MCU_DEBUG"], "Hello from MCU!".encode("utf-8") + b"\0"))

        poller = zmq.Poller()
        poller.register(socket, zmq.POLLIN)

        while not done:
            evts = poller.poll(100)
            if len(evts) == 0:
                continue
            data = socket.recv()
            if int(data[0]) == IDS["IPP_APP_DAC_SET"]:
                value = decode_dac_set(data)
            elif int(data[0]) == IDS["IPP_APP_ADC_REQ"]:
                index = decode_adc_req(data)
                if index == 1:
                    socket.send(encode_adc_val(1, value))
                elif index == 2:
                    socket.send(encode_adc_val(2, max_val))
                else:
                    raise Exception("Unexpected ADC index")
            else:
                raise Exception("Unexpected message type")

    thread = Thread(target=worker)
    thread.start()

    with ca.Repeater(prefix), ioc:
        time.sleep(0.2)
        assert_eq(ca.get(prefix, "ai1"), 0)
        assert_eq(ca.get(prefix, "ai2"), max_val)

        ca.put(prefix, "ao0", some_val)

        time.sleep(0.2)
        assert_eq(ca.get(prefix, "ai1"), some_val)
        assert_eq(ca.get(prefix, "ai2"), max_val)

    done = True
    thread.join()

    print("Test passed!")
