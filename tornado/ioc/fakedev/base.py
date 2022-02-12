from __future__ import annotations
import chunk
from dataclasses import dataclass
from typing import Any, List

import asyncio
from threading import Thread
from pathlib import Path
import logging

import zmq
import zmq.asyncio as azmq

from ferrite.utils.epics.ioc import Ioc
import ferrite.utils.epics.ca as ca

from tornado.ipp import AppMsg, McuMsg


def dac_code_to_volt(code: int) -> float:
    return (code - 32767) * (315.7445 * 1e-6)


def adc_volt_to_code(voltage: float) -> int:
    return round(voltage / (346.8012 * 1e-6) * 256)


async def _send_msg(socket: azmq.Socket, msg: McuMsg.Variant) -> None:
    await socket.send(McuMsg(msg).store())


async def _recv_msg(socket: azmq.Socket) -> AppMsg:
    data = await socket.recv()
    assert isinstance(data, bytes)
    return AppMsg.load(data)


class FakeDev:

    @dataclass
    class Handler:
        adc_count = 0

        # Takes DAC value and returns ADC values
        def transfer(self, dac: float) -> List[float]:
            raise NotImplementedError()

        def transfer_codes(self, dac_code: int) -> List[int]:
            return [adc_volt_to_code(adc_code) for adc_code in self.transfer(dac_code_to_volt(dac_code))]

    @dataclass
    class Config:
        adc_count: int
        sample_period: float
        chunk_size: int

    @staticmethod
    def default_config() -> Config:
        return FakeDev.Config(
            adc_count=6,
            sample_period=0.0001, # 10 kHz
            chunk_size=97, # random prime, PV size should be non-multple of it
        )

    def __init__(self, prefix: Path, ioc: Ioc, config: Config, handler: FakeDev.Handler) -> None:
        self.prefix = prefix
        self.ioc = ioc

        self.context = azmq.Context()
        self.socket = self.context.socket(zmq.PAIR)

        self.config = config
        self.handler = handler

        self.adc_wfs: List[List[int]] = [[] for _ in range(self.config.adc_count)]

    async def sample(self, dac: int) -> None:
        adcs = self.handler.transfer_codes(dac)
        assert len(adcs) == len(self.adc_wfs)
        for adc, wf in zip(adcs, self.adc_wfs):
            wf.append(adc)

        # Send back ADC chunks if they're ready
        chunk_size = self.config.chunk_size
        if len(self.adc_wfs[0]) >= chunk_size:
            for i, wf in enumerate(self.adc_wfs):
                await _send_msg(self.socket, McuMsg.AdcWf(i, wf[:chunk_size]))
            self.adc_wfs = [wf[chunk_size:] for wf in self.adc_wfs]

    async def sample_chunk(self, dac_wf: List[int]) -> None:

        async def sample_all() -> None:
            for dac in dac_wf:
                await self.sample(dac)

        await asyncio.gather(
            sample_all(),
            asyncio.sleep(self.config.sample_period * len(dac_wf)),
        )

    async def loop(self) -> None:
        assert isinstance((await _recv_msg(self.socket)).variant, AppMsg.Start)
        logging.info("Received start signal")
        await _send_msg(self.socket, McuMsg.Debug("Hello from MCU!"))

        await _send_msg(self.socket, McuMsg.DacWfReq())
        while True:
            msg = await _recv_msg(self.socket)
            if isinstance(msg.variant, AppMsg.DacWf):
                await self.sample_chunk(msg.variant.elements)
                await _send_msg(self.socket, McuMsg.DacWfReq())
            else:
                raise Exception("Unexpected message type")

    async def run(self) -> None:
        self.socket.bind("tcp://127.0.0.1:8321")
        with ca.Repeater(self.prefix), self.ioc:
            logging.debug("Fakedev started")
            await self.loop()
