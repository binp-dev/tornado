from __future__ import annotations
from typing import List

import asyncio
from pathlib import Path
from dataclasses import dataclass

import zmq
import zmq.asyncio as azmq

from ferrite.utils.epics.ioc import Ioc

from tornado.ipp import AppMsg, McuMsg
from tornado.common.config import Config as CommonConfig

import logging

logger = logging.getLogger(__name__)


async def _send_msg(socket: azmq.Socket, msg: McuMsg.Variant) -> None:
    await socket.send(McuMsg(msg).store())


async def _recv_msg(socket: azmq.Socket) -> AppMsg:
    data = await socket.recv()
    assert isinstance(data, bytes)
    return AppMsg.load(data)


class FakeDev:

    @dataclass
    class Handler:
        config: CommonConfig

        def dac_code_to_volt(self, code: int) -> float:
            return (code - self.config.dac_shift) * (self.config.dac_step_uv * 1e-6)

        def adc_volt_to_code(self, voltage: float) -> int:
            return round(voltage / (self.config.adc_step_uv * 1e-6) * 256)

        # Takes DAC value and returns ADC values
        def transfer(self, dac: float) -> List[float]:
            raise NotImplementedError()

        def transfer_codes(self, dac_code: int) -> List[int]:
            return [self.adc_volt_to_code(adc_code) for adc_code in self.transfer(self.dac_code_to_volt(dac_code))]

    @dataclass
    class Config:
        adc_count: int
        sample_period: float
        chunk_size: int
        keepalive_timeout: float

    @staticmethod
    def _make_config(cc: CommonConfig) -> Config:
        return FakeDev.Config(
            adc_count=cc.adc_count,
            sample_period=0.0001, # 10 kHz
            chunk_size=(cc.rpmsg_max_msg_len - 3) // 4,
            keepalive_timeout=(cc.keep_alive_max_delay_ms / 1000),
        )

    def __init__(self, ioc: Ioc, cc: CommonConfig, handler: FakeDev.Handler) -> None:
        self.ioc = ioc

        self.context = azmq.Context()
        self.socket = self.context.socket(zmq.PAIR)

        self.config = FakeDev._make_config(cc)
        self.handler = handler

        self.adc_buffers: List[List[int]] = [[] for _ in range(self.config.adc_count)]

    async def _sample(self, dac: int) -> None:
        adcs = self.handler.transfer_codes(dac)
        assert len(adcs) == len(self.adc_buffers)
        for adc, wf in zip(adcs, self.adc_buffers):
            wf.append(adc)

        # Send back ADC chunks if they're ready
        chunk_size = self.config.chunk_size
        if len(self.adc_buffers[0]) >= chunk_size:
            for i, wf in enumerate(self.adc_buffers):
                await _send_msg(self.socket, McuMsg.AdcData(i, wf[:chunk_size]))
            self.adc_buffers = [wf[chunk_size:] for wf in self.adc_buffers]

    async def _sample_chunk(self, dac_chunk: List[int]) -> None:

        async def sample_all() -> None:
            for dac in dac_chunk:
                await self._sample(dac)

        await asyncio.gather(
            sample_all(),
            asyncio.sleep(self.config.sample_period * len(dac_chunk)),
        )

    async def _recv_msg(self) -> None:
        base_msg = await _recv_msg(self.socket)
        msg = base_msg.variant
        if isinstance(msg, AppMsg.DacData):
            await self._sample_chunk(msg.points)
            await _send_msg(self.socket, McuMsg.DacRequest(len(msg.points)))
        elif isinstance(msg, AppMsg.DacMode):
            if msg.enable:
                logger.debug("Start Dac")
            else:
                logger.debug("Stop Dac")
        elif isinstance(msg, AppMsg.KeepAlive):
            pass
        else:
            raise RuntimeError(f"Unexpected message type")

    async def loop(self) -> None:
        assert isinstance((await _recv_msg(self.socket)).variant, AppMsg.Connect)
        logger.info("IOC connected signal")
        await _send_msg(self.socket, McuMsg.Debug("Hello from MCU!"))

        await _send_msg(self.socket, McuMsg.DacRequest(self.config.chunk_size))
        while True:
            try:
                await asyncio.wait_for(self._recv_msg(), self.config.keepalive_timeout)
            except asyncio.TimeoutError:
                logger.error("Keep-alive timeout reached")
                raise

    async def run(self) -> None:
        self.socket.bind("tcp://127.0.0.1:8321")
        with self.ioc:
            await asyncio.sleep(1.0)
            logger.debug("Fakedev started")
            await self.loop()
