from __future__ import annotations
from typing import List

import asyncio
from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

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

        def dac_codes_to_volts(self, codes: NDArray[np.int32]) -> NDArray[np.float64]:
            array: NDArray[np.float64] = codes.astype(np.float64)
            return (array - self.config.dac_code_shift) * (self.config.dac_step_uv * 1e-6)

        def adc_volts_to_codes(self, volts: NDArray[np.float64]) -> NDArray[np.int32]:
            return (volts / (self.config.adc_step_uv * 1e-6) * 256).astype(np.int32)

        # Takes DAC values and returns new ADC values for all channels
        async def transfer(self, dac: NDArray[np.float64]) -> NDArray[np.float64]:
            raise NotImplementedError()

        async def transfer_codes(self, dac_codes: NDArray[np.int32]) -> NDArray[np.int32]:
            adcs = await self.transfer(self.dac_codes_to_volts(dac_codes))
            return self.adc_volts_to_codes(adcs)

    def __init__(self, ioc: Ioc, config: Config, handler: FakeDev.Handler) -> None:
        self.ioc = ioc

        self.context = azmq.Context()
        self.socket = self.context.socket(zmq.PAIR)

        self.config = config
        self.handler = handler

    async def _sample_chunk(self, dac: NDArray[np.int32]) -> None:
        adcs = await self.handler.transfer_codes(dac)
        await _send_msg(self.socket, McuMsg.AdcData(adcs))
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
