# type: ignore
from __future__ import annotations
import re
from sys import prefix
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
    _adc_wf_msg_max_elems = 10

    class Handler:

        def write_dac(self, voltage: float) -> None:
            raise NotImplementedError()

        def read_adcs(self) -> List[float]:
            raise NotImplementedError()

        def write_dac_code(self, code: int) -> None:
            self.write_dac(dac_code_to_volt(code))

        def read_adc_codes(self) -> List[int]:
            return [adc_volt_to_code(v) for v in self.read_adcs()]

        def read_adc_wfs(self) -> List[int]:
            return self.adc_wfs

        def _fill_dac_wf_buff(self, dac_wf_buff, dac_wf_data, dac_wf_data_pos) -> int:
            elems_to_fill = self.dac_wf_size - len(dac_wf_buff)
            elems_left = len(dac_wf_data) - dac_wf_data_pos
            if (elems_left < elems_to_fill):
                elems_to_fill = elems_left

            dac_wf_buff += dac_wf_data[dac_wf_data_pos : dac_wf_data_pos + elems_to_fill]
            return elems_to_fill

        def write_dac_wf(self, dac_wf_data) -> None:
            if len(self.dac_wfs) == 0:
                self.dac_wfs.append([])

            dac_wf_data_pos = 0
            dac_wf_buff = self.dac_wfs[len(self.dac_wfs) - 1]
            while dac_wf_data_pos < len(dac_wf_data):
                if len(dac_wf_buff) == self.dac_wf_size:
                    self.dac_wfs.append([])
                    dac_wf_buff = self.dac_wfs[len(self.dac_wfs) - 1]

                dac_wf_data_pos += self._fill_dac_wf_buff(dac_wf_buff, dac_wf_data, dac_wf_data_pos)

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

        adc_wf_positions = [0 for i in range(self.handler.adc_count)]
        
        _send_msg(self.socket, McuMsg.DacWfReq())

        while not self.done:
            evts = poller.poll(100)

            for i in range(len(self.handler.read_adc_wfs())):
                adc_wf = self.handler.read_adc_wfs()[i]
                if adc_wf_positions[i] == len(adc_wf):
                    continue

                adc_wf_msg_data = []
                adc_wf_positions[i] += self._fill_adc_wf_msg_buff(adc_wf_msg_data, adc_wf, adc_wf_positions[i])
                _send_msg(self.socket, McuMsg.AdcWf(i, adc_wf_msg_data))
            
            if len(evts) == 0:
                continue
            msg = AppMsg.load(self.socket.recv())
            if msg.variant.is_instance_of(AppMsg.DacSet):
                self.handler.write_dac_code(msg.variant.value)
            elif msg.variant.is_instance_of(AppMsg.AdcReq):
                adcs = self.handler.read_adc_codes()
                _send_msg(self.socket, McuMsg.AdcVal(adcs))
            elif msg.variant.is_instance_of(AppMsg.DacWf):
                self.handler.write_dac_wf(msg.variant.elements)
                _send_msg(self.socket, McuMsg.DacWfReq())
            else:
                raise Exception("Unexpected message type")

    def _fill_adc_wf_msg_buff(self, buff, adc_wf, adc_wf_position) -> int:
        elems_to_send = self._adc_wf_msg_max_elems
        elems_to_fill = len(adc_wf) - adc_wf_position
        if elems_to_fill < elems_to_send:
            elems_to_send = elems_to_fill

        buff += adc_wf[adc_wf_position : adc_wf_position + elems_to_send]
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
