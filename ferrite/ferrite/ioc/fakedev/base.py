# type: ignore
from __future__ import annotations
from typing import Any, List, Optional

import zmq
from threading import Thread
from pathlib import Path

from ferrite.ipp import AppMsg, McuMsg
from ferrite.utils.epics.ioc import Ioc
from ferrite.codegen.variant import VariantValue
import ferrite.utils.epics.ca as ca


def dac_code_to_volt(code: int) -> float:
    return (code - 32767) * (315.7445 * 1e-6)


def adc_volt_to_code(voltage: float) -> int:
    return round(voltage / (346.8012 * 1e-6) * 256)


def _send_msg(socket: zmq.Socket, msg: bytes) -> None:
    any_msg = None
    for i, f in enumerate(McuMsg.variants):
        if f.type.is_instance(msg):
            any_msg = McuMsg(i, msg)
            break
    assert any_msg is not None
    socket.send(any_msg.store())


def _recv_msg(socket: zmq.Socket) -> VariantValue:
    return AppMsg.load(socket.recv())


class FakeDev:

    class Handler:

        def write_dac(self, voltage: float) -> None:
            raise NotImplementedError()

        def read_adcs(self) -> List[float]:
            raise NotImplementedError()

        def write_dac_code(self, code: int) -> None:
            self.write_dac(dac_code_to_volt(code))

        def read_adc_codes(self) -> List[int]:
            return [adc_volt_to_code(v) for v in self.read_adcs()]

    def __init__(self, prefix: Path, ioc: Ioc, handler: FakeDev.Handler) -> None:
        self.prefix = prefix
        self.ca_repeater = ca.Repeater(prefix)
        self.ioc = ioc

        self.context = zmq.Context()
        self.socket: zmq.Socket = self.context.socket(zmq.PAIR)

        self.handler = handler
        self.done = True
        self.thread = Thread(target=lambda: self._dev_loop())

    def _dev_loop(self) -> None:
        assert _recv_msg(self.socket).variant.is_instance_of(AppMsg.Start)
        print("Received start signal")
        _send_msg(self.socket, McuMsg.Debug("Hello from MCU!"))

        poller = zmq.Poller()
        poller.register(self.socket, zmq.POLLIN)

        while not self.done:
            evts = poller.poll(100)
            if len(evts) == 0:
                continue
            msg = AppMsg.load(self.socket.recv())
            if msg.variant.is_instance_of(AppMsg.DacSet):
                self.handler.write_dac_code(msg.variant.value)
            elif msg.variant.is_instance_of(AppMsg.AdcReq):
                adcs = self.handler.read_adc_codes()
                _send_msg(self.socket, McuMsg.AdcVal(adcs))
            else:
                raise Exception("Unexpected message type")

    def __enter__(self) -> None:
        self.socket.bind("tcp://127.0.0.1:8321")

        self.ca_repeater.__enter__()
        self.ioc.__enter__()

        self.done = False
        self.thread.start()

    def __exit__(self, *args: Any) -> None:
        self.done = True
        self.thread.join()

        self.ioc.__exit__(*args)
        self.ca_repeater.__exit__(*args)

        self.socket.close()
