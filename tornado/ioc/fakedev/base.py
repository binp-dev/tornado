from __future__ import annotations
from typing import List

import asyncio
from pathlib import Path
from dataclasses import dataclass

import zmq
import zmq.asyncio as azmq

from ferrite.utils.epics.ioc import Ioc

from tornado.ipp import AppMsg, McuMsg
from tornado.common.config import read_common_config

import logging

logger = logging.getLogger(__name__)


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
        keepalive_timeout: float

    @staticmethod
    def read_config(source: Path) -> Config:
        cc = read_common_config(source)
        return FakeDev.Config(
            adc_count=cc.adc_count,
            sample_period=0.0001, # 10 kHz
            chunk_size=(cc.rpmsg_max_msg_len - 3) // 4,
            keepalive_timeout=(cc.keep_alive_max_delay_ms / 1000),
        )

    def __init__(self, ioc: Ioc, config: Config, handler: FakeDev.Handler) -> None:
        self.ioc = ioc

        self.context = azmq.Context()
        self.socket = self.context.socket(zmq.PAIR)

        self.config = config
        self.handler = handler

        self.adc_wfs: List[List[int]] = [[] for _ in range(self.config.adc_count)]

    async def _sample(self, dac: int) -> None:
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

    async def _sample_chunk(self, dac_wf: List[int]) -> None:

        async def sample_all() -> None:
            for dac in dac_wf:
                await self._sample(dac)

        await asyncio.gather(
            sample_all(),
            asyncio.sleep(self.config.sample_period * len(dac_wf)),
        )

    async def _recv_msg(self) -> None:
        msg = await _recv_msg(self.socket)
        if isinstance(msg.variant, AppMsg.DacWf):
            await self._sample_chunk(msg.variant.elements)
            await _send_msg(self.socket, McuMsg.DacWfReq(self.config.chunk_size))
        elif isinstance(msg.variant, AppMsg.StartDac):
            logger.debug("StartDac")
        elif isinstance(msg.variant, AppMsg.KeepAlive):
            pass
        else:
            raise RuntimeError(f"Unexpected message type")

    async def loop(self) -> None:
        assert isinstance((await _recv_msg(self.socket)).variant, AppMsg.Connect)
        logger.info("Received start signal")
        await _send_msg(self.socket, McuMsg.Debug("Hello from MCU!"))

        await _send_msg(self.socket, McuMsg.DacWfReq(self.config.chunk_size))
        while True:
            try:
                await asyncio.wait_for(self._recv_msg(), self.config.keepalive_timeout)
            except asyncio.TimeoutError:
                logger.error("Keep-alive timeout reached")
                raise

    async def run(self) -> None:
        self.socket.bind("tcp://127.0.0.1:8321")
        with self.ioc:
            logger.debug("Fakedev started")
            await self.loop()
