from __future__ import annotations
from typing import List

import asyncio
from pathlib import Path
from dataclasses import dataclass

import zmq
import zmq.asyncio as azmq

from ferrite.utils.epics.ioc import Ioc

from tornado.ipp import AppMsg, McuMsg
from tornado.common.config import Config

import logging

logger = logging.getLogger(__name__)


async def _send_msg(socket: azmq.Socket, msg: McuMsg.Variant) -> None:
    await socket.send(McuMsg(msg).store())


async def _recv_msg(socket: azmq.Socket) -> AppMsg:
    data = await socket.recv()
    assert isinstance(data, bytes)
    return AppMsg.load(data)


class FakeDev:
    REQUEST_SIZE: int = 1024

    @dataclass
    class Handler:
        config: Config

        def dac_code_to_volt(self, code: int) -> float:
            return (code - self.config.dac_code_shift) * (self.config.dac_step_uv * 1e-6)

        def adc_volt_to_code(self, voltage: float) -> int:
            return round(voltage / (self.config.adc_step_uv * 1e-6) * 256)

        # Takes DAC value and returns ADC values
        def transfer(self, dac: float) -> List[float]:
            raise NotImplementedError()

        def transfer_codes(self, dac_code: int) -> List[int]:
            return [self.adc_volt_to_code(adc_code) for adc_code in self.transfer(self.dac_code_to_volt(dac_code))]

    def __init__(self, ioc: Ioc, config: Config, handler: FakeDev.Handler) -> None:
        self.ioc = ioc

        self.context = azmq.Context()
        self.socket = self.context.socket(zmq.PAIR)

        self.config = config
        self.handler = handler

        self.adc_buffers: List[List[int]] = [[] for _ in range(self.config.adc_count)]

    async def _sample_chunk(self, dac: List[int]) -> None:
        adcs = zip(*[self.handler.transfer_codes(x) for x in dac])
        for i, adc in enumerate(adcs):
            await _send_msg(self.socket, McuMsg.AdcData(i, list(adc)))
        await _send_msg(self.socket, McuMsg.DacRequest(len(dac)))

    async def _recv_msg(self) -> None:
        base_msg = await _recv_msg(self.socket)
        msg = base_msg.variant
        if isinstance(msg, AppMsg.DacData):
            await self._sample_chunk(msg.points)
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

        await _send_msg(self.socket, McuMsg.DacRequest(FakeDev.REQUEST_SIZE))
        while True:
            try:
                await asyncio.wait_for(self._recv_msg(), self.config.keep_alive_max_delay_ms * 1e-3)
            except asyncio.TimeoutError:
                logger.error("Keep-alive timeout reached")
                raise

    async def run(self) -> None:
        self.socket.bind("tcp://127.0.0.1:8321")
        with self.ioc:
            await asyncio.sleep(1.0)
            logger.debug("Fakedev started")
            await self.loop()
