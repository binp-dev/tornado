from __future__ import annotations
from typing import List

import os
import time
import zmq
from threading import Thread

from ferrite.ipp import AppMsg, McuMsg
from ferrite.utils.epics.ioc import Ioc
import ferrite.utils.epics.ca as ca


def assert_eq(a: float, b: float, eps: float = 1e2) -> None:
    assert abs(a - b) < eps


def send_msg(socket, msg) -> None:
    any_msg = None
    for i, f in enumerate(McuMsg.variants):
        if f.type.is_instance(msg):
            any_msg = McuMsg(i, msg)
            break
    assert any_msg is not None
    socket.send(any_msg.store())


def recv_msg(socket):
    return AppMsg.load(socket.recv())


def run_test(
    epics_base_dir,
    ioc_dir,
    ipp_dir,
    arch,
):
    global IDS

    ioc = Ioc(os.path.join(ioc_dir, "bin", arch, "PSC"), os.path.join(ioc_dir, "iocBoot/iocPSC/st.cmd"))

    context = zmq.Context()
    socket = context.socket(zmq.PAIR)
    socket.bind("tcp://127.0.0.1:8321")

    prefix = os.path.join(epics_base_dir, "bin", arch)

    global done
    global value
    done = False
    value = 0

    max_val = 0x7FFFFFFF
    min_val = -0x80000000
    some_val = 0x789ABCDE
    values = [1, -1, some_val, max_val, min_val]

    def worker():
        global done
        global value

        assert recv_msg(socket).variant.is_instance(AppMsg.Start)
        print("Received start signal")
        send_msg(socket, McuMsg.Debug("Hello from MCU!"))

        poller = zmq.Poller()
        poller.register(socket, zmq.POLLIN)

        while not done:
            evts = poller.poll(100)
            if len(evts) == 0:
                continue
            msg = AppMsg.load(socket.recv())
            if msg.variant.is_instance(AppMsg.DacSet):
                value = msg.variant.value
            elif msg.variant.is_instance(AppMsg.AdcReq):
                send_msg(socket, McuMsg.AdcVal([value] + values))
            else:
                raise Exception("Unexpected message type")

    thread = Thread(target=worker)
    thread.start()

    with ca.Repeater(prefix), ioc:
        time.sleep(0.2)
        assert_eq(ca.get(prefix, f"ai0"), 0)
        for i in range(5):
            assert_eq(ca.get(prefix, f"ai{i + 1}"), values[i])

        ca.put(prefix, "ao0", some_val)

        print("Waiting for record to scan ...")
        time.sleep(2.0)

        assert_eq(ca.get(prefix, f"ai0"), some_val)
        for i in range(5):
            assert_eq(ca.get(prefix, f"ai{i + 1}"), values[i])

    done = True
    thread.join()

    print("Test passed!")
