import os
import time
import zmq

from utils.epics.ioc import Ioc
from utils.epics import ca
import utils.proto as proto

IDS = None

def decode_wf_data(data, nbytes=3, dlen=256):
    cmd = int(data[0])
    assert(cmd == IDS["PSCA_WF_DATA"])
    assert(int(data[1]) == len(data) - 2)

    arr = data[2:]
    arr = [arr[i : i + nbytes] for i in range(0, len(arr), nbytes)]
    assert(all([len(a) == nbytes for a in arr]))
    arr = [int.from_bytes(a, "little") for a in arr]
    return arr

def encode(id, data=b""):
    assert(len(data) <= 256 - 2)
    return bytes([id, len(data)]) + data

def assert_pop(queue, arr):
    print(arr)
    assert len(queue) >= len(arr)
    try:
        assert all((a == b for a, b in zip(queue, arr)))
    except AssertionError:
        print("{} != {}".format(queue[:len(arr)], arr))
        raise
    return queue[len(arr):]

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

    wfs = 200
    with ca.Repeater(prefix), ioc:
        start_msg = socket.recv()
        assert(int(start_msg[0]) == IDS["PSCA_START"])
        assert(int(start_msg[1]) == 0)
        print("Received start signal")

        socket.send(encode(IDS["PSCM_MESSAGE"], "Hello from MCU!".encode("utf-8")))

        # Empty PV
        queue = [0]*wfs*2
        for _ in range(4):
            socket.send(encode(IDS["PSCM_WF_REQ"]))
            queue = assert_pop(queue, decode_wf_data(socket.recv()))
        
        # Linear ascend
        wf = list(range(wfs))
        ca.put(prefix, "WAVEFORM", wf, array=True)
        queue += wf*2
        for _ in range(5):
            socket.send(encode(IDS["PSCM_WF_REQ"]))
            queue = assert_pop(queue, decode_wf_data(socket.recv()))

        # Linear descend
        wf = [wfs - x - 1 for x in range(wfs)]
        ca.put(prefix, "WAVEFORM", wf, array=True)
        queue += wf*2
        for _ in range(5):
            socket.send(encode(IDS["PSCM_WF_REQ"]))
            queue = assert_pop(queue, decode_wf_data(socket.recv()))

        # Double write
        wf = [wfs + x for x in range(wfs)]
        ca.put(prefix, "WAVEFORM", range(0, 2*wfs, 2), array=True)
        ca.put(prefix, "WAVEFORM", wf, array=True)
        queue += wf*4
        for _ in range(9):
            socket.send(encode(IDS["PSCM_WF_REQ"]))
            queue = assert_pop(queue, decode_wf_data(socket.recv()))

    print("Test passed!")
