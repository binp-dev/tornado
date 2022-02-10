from __future__ import annotations
from dataclasses import dataclass
from typing import Any, List, Optional

import zmq
from threading import Thread
from pathlib import Path

from ferrite.utils.epics.ioc import Ioc
from ferrite.codegen.variant import VariantValue
import ferrite.utils.epics.ca as ca

from tornado.ipp import AppMsg, McuMsg


def dac_code_to_volt(code: int) -> float:
    return (code - 32767) * (315.7445 * 1e-6)


def adc_volt_to_code(voltage: float) -> int:
    return round(voltage / (346.8012 * 1e-6) * 256)


def _send_msg(socket: zmq.Socket, msg: McuMsg.Variant) -> None:
    socket.send(McuMsg(msg).store())


def _recv_msg(socket: zmq.Socket) -> AppMsg:
    return AppMsg.load(socket.recv())


class FakeDev:
    adc_wf_msg_max_elems = 63 # FIXME: Remove
    poll_ms_timeout = 100 # FIXME: Remove

    @dataclass
    class Handler:
        adc_count = 0

        def write_dac_wf(self, wf: List[int]) -> None:
            raise NotImplementedError()

        def read_adc_wfs(self) -> List[List[int]]:
            raise NotImplementedError()

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
        assert isinstance(_recv_msg(self.socket).variant, AppMsg.Start)
        print("Received start signal")
        _send_msg(self.socket, McuMsg.Debug("Hello from MCU!"))

        poller = zmq.Poller()
        poller.register(self.socket, zmq.POLLIN)

        adc_wf_positions = [0 for i in range(self.handler.adc_count)]

        _send_msg(self.socket, McuMsg.DacWfReq())

        while not self.done:
            evts = poller.poll(self.poll_ms_timeout)

            for i, adc_wf in enumerate(self.handler.read_adc_wfs()):
                if adc_wf_positions[i] == len(adc_wf):
                    continue

                adc_wf_msg_data: List[int] = []
                adc_wf_positions[i] += self._fill_adc_wf_msg_buff(adc_wf_msg_data, adc_wf, adc_wf_positions[i])
                _send_msg(self.socket, McuMsg.AdcWf(i, adc_wf_msg_data))

            if len(evts) == 0:
                continue
            msg = AppMsg.load(self.socket.recv())
            if isinstance(msg.variant, AppMsg.DacWf):
                self.handler.write_dac_wf(msg.variant.elements)
                _send_msg(self.socket, McuMsg.DacWfReq())
            else:
                raise Exception("Unexpected message type")

    def _fill_adc_wf_msg_buff(self, buff: List[int], adc_wf: List[int], adc_wf_position: int) -> int:
        elems_to_send = self.adc_wf_msg_max_elems
        elems_to_fill = len(adc_wf) - adc_wf_position
        if elems_to_fill < elems_to_send:
            elems_to_send = elems_to_fill

        buff += adc_wf[adc_wf_position:adc_wf_position + elems_to_send]
        return elems_to_send

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
