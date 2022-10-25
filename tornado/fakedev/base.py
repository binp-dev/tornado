from __future__ import annotations
from typing import Awaitable, ClassVar

from dataclasses import dataclass
import asyncio

import numpy as np
from numpy.typing import NDArray

from ferrite.utils.asyncio.task import forever, with_background
from ferrite.utils.asyncio.net import TcpListener, MsgWriter, MsgReader
from ferrite.utils.epics.ioc import AsyncIoc

from tornado.ipp import AppMsg, McuMsg
import tornado.config as config

import logging

logger = logging.getLogger(__name__)


@dataclass
class FakeDev:
    request_size: ClassVar[int] = 1024

    ioc: AsyncIoc
    handler: FakeDev.Handler

    @dataclass
    class Handler:

        def dac_codes_to_volts(self, codes: NDArray[np.int32]) -> NDArray[np.float64]:
            array: NDArray[np.float64] = codes.astype(np.float64)
            return (array - config.DAC_CODE_SHIFT) * (config.DAC_STEP_UV * 1e-6)

        def adc_volts_to_codes(self, volts: NDArray[np.float64]) -> NDArray[np.int32]:
            return (volts / (config.ADC_STEP_UV * 1e-6) * 256).astype(np.int32)

        # Takes DAC values and returns new ADC values for all channels
        async def transfer(self, dac: NDArray[np.float64]) -> NDArray[np.float64]:
            raise NotImplementedError()

        async def transfer_codes(self, dac_codes: NDArray[np.int32]) -> NDArray[np.int32]:
            adcs = await self.transfer(self.dac_codes_to_volts(dac_codes))
            return self.adc_volts_to_codes(adcs)

    async def _send_msg(self, msg: McuMsg.Variant) -> None:
        #logger.debug(f"send_msg: {msg._type.name}")
        await self.writer.write_msg(McuMsg(msg))

    async def _recv_msg(self) -> AppMsg:
        msg = await self.reader.read_msg()
        #logger.debug(f"recv_msg: {msg.variant._type.name}")
        return msg

    async def _sample_chunk(self, dac: NDArray[np.int32]) -> None:
        adcs = await self.handler.transfer_codes(dac)

        points_in_msg = (config.MAX_MCU_MSG_LEN - 3) // (4 * config.ADC_COUNT)
        #logger.debug(f"points_in_msg: {points_in_msg}")
        while len(adcs) > points_in_msg:
            await self._send_msg(McuMsg.AdcData(adcs[:points_in_msg]))
            adcs = adcs[points_in_msg:]
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
                await asyncio.wait_for(self._recv_and_handle_msg(), config.KEEP_ALIVE_MAX_DELAY_MS * 1e-3)
            except asyncio.TimeoutError:
                logger.error("Keep-alive timeout reached")
                raise

    async def run_with(self, inner: Awaitable[None]) -> None:
        async with TcpListener("127.0.0.1", 8321) as lis:
            async with self.ioc:
                async for stream in lis:
                    self.writer = MsgWriter(McuMsg, stream.writer)
                    self.reader = MsgReader(AppMsg, stream.reader, config.MAX_APP_MSG_LEN)
                    break
                logger.debug("Fakedev started")
                await with_background(inner, self._loop())

    async def run(self) -> None:
        await self.run_with(forever())
