from __future__ import annotations
from typing import ClassVar

import asyncio
from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

from ferrite.utils.asyncio.net import TcpListener, MsgWriter, MsgReader
from ferrite.utils.epics.ioc import AsyncIoc

from tornado.ipp import AppMsg, McuMsg
from tornado.common.config import Config

import logging

logger = logging.getLogger(__name__)


@dataclass
class FakeDev:
    request_size: ClassVar[int] = 1024

    ioc: AsyncIoc
    config: Config
    handler: FakeDev.Handler

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

    async def _send_msg(self, msg: McuMsg.Variant) -> None:
        await self.writer.write_msg(McuMsg(msg))

    async def _recv_msg(self) -> AppMsg:
        return await self.reader.read_msg()

    async def _sample_chunk(self, dac: NDArray[np.int32]) -> None:
        adcs = await self.handler.transfer_codes(dac)
        await self._send_msg(McuMsg.AdcData(adcs))
        await self._send_msg(McuMsg.DacRequest(len(dac)))

    async def _recv_and_handle_msg(self) -> None:
        base_msg = await self._recv_msg()
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

    async def _loop(self) -> None:
        assert isinstance((await self._recv_msg()).variant, AppMsg.Connect)
        logger.info("IOC connected signal")
        await self._send_msg(McuMsg.Debug("Hello from MCU!"))

        await self._send_msg(McuMsg.DacRequest(self.request_size))
        while True:
            try:
                await asyncio.wait_for(self._recv_and_handle_msg(), self.config.keep_alive_max_delay_ms * 1e-3)
            except asyncio.TimeoutError:
                logger.error("Keep-alive timeout reached")
                raise

    async def run(self) -> None:
        async with TcpListener("127.0.0.1", 8321) as lis:
            async with self.ioc:
                async for stream in lis:
                    self.writer = MsgWriter(McuMsg, stream.writer)
                    self.reader = MsgReader(AppMsg, stream.reader, self.config.rpmsg_max_app_msg_len)
                    break
                logger.debug("Fakedev started")
                await self._loop()
