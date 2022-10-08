from __future__ import annotations
from typing import Awaitable, Callable, Dict

import os
from pathlib import Path
from dataclasses import dataclass
import asyncio

import numpy as np
from numpy.typing import NDArray

from ferrite.utils.asyncio.task import with_background
from ferrite.utils.epics.ioc import AsyncIoc, RunMode
from ferrite.utils.epics.pv import Ca, Pv, PvMonitor, PvType
from ferrite.utils.progress import CountBar
import ferrite.utils.epics.ca as ca

from tornado.common.config import Config, read_common_config
from tornado.fakedev.base import FakeDev

import logging

logger = logging.getLogger(__name__)


def array_approx_eq(a: NDArray[np.float64], b: NDArray[np.float64], eps: float = 1e-3) -> bool:
    return float(np.max(np.abs(a - b))) <= eps


@dataclass
class Handler(FakeDev.Handler):
    dt: float = 0.0

    class Waveform:

        def __init__(self) -> None:
            self.data: NDArray[np.float64] = np.empty(0, dtype=np.float64)
            self.notify_on: int | None = None
            self.event = asyncio.Event()

        def push(self, chunk: NDArray[np.float64]) -> None:
            self.data = np.append(self.data, chunk)
            if self.notify_on is not None and self.notify_on <= len(self.data):
                self.notify_on = None
                self.event.set()

        async def pop(self, size: int) -> NDArray[np.float64]:
            if len(self.data) < size:
                assert self.notify_on is None
                self.notify_on = size
                await self.event.wait()
                self.event.clear()
                assert self.notify_on is None
            assert len(self.data) >= size
            chunk: NDArray[np.float64] = self.data[:size]
            self.data = self.data[size:]
            return chunk

        async def pop_check(self, chunk: NDArray[np.float64]) -> None:
            array_approx_eq(await self.pop(len(chunk)), chunk)

    def __post_init__(self) -> None:
        self.dac = Handler.Waveform()
        self.adcs = [Handler.Waveform() for _ in range(self.config.adc_count + 1)]

    async def transfer(self, dac: NDArray[np.float64]) -> NDArray[np.float64]:
        self.dac.push(dac)

        adcs = [dac / self.config.dac_max_abs_v * self.config.adc_max_abs_v]
        step = 1e-4
        for i in range(1, self.config.adc_count):
            array = self.dt + step * np.arange(len(dac), dtype=np.float64)
            adcs.append(self.config.adc_max_abs_v * np.sin(2.0 * np.pi * i * array))
        self.dt += step * len(dac)

        for adc, chunk in zip(self.adcs, adcs):
            adc.push(chunk)

        result: NDArray[np.float64] = np.stack(adcs).transpose()
        return result


async def test(config: Config, handler: Handler) -> None:
    ca = Ca(timeout=2.0)
    aais = await asyncio.gather(*[ca.connect(f"aai{i}", PvType.ARRAY_FLOAT, monitor=True) for i in range(config.adc_count)])
    aao = await ca.connect("aao0", PvType.ARRAY_FLOAT)
    aao_request = await ca.connect("aao0_request", PvType.BOOL, monitor=True)
    aao_mode = await ca.connect("aao0_mode", PvType.BOOL)

    wf_size = aao.nelm
    logger.debug(f"Waveform max size: {wf_size}")
    # Check that `aai*` sizes are the same as `aao0` size
    assert all([wf_size == aai.nelm for aai in aais])

    async def write_and_check_dac(array: NDArray[np.float64]) -> None:
        await aao.put(array)
        await handler.dac.pop_check(array)
        #logger.debug(f"DAC of size {len(array)} is correct")

    adcs_samples_count = [0] * config.adc_count

    async def watch_single_adc(index: int, adc_pv: PvMonitor[NDArray[np.float64]]) -> None:
        async for array in adc_pv:
            if len(array) == 0:
                continue
            await handler.adcs[index].pop_check(array)
            adcs_samples_count[index] += len(array)
            #logger.debug(f"ADC[{index}] of size {len(array)} is correct")

    async def watch_adcs() -> None:
        await asyncio.gather(*[watch_single_adc(i, pv) for i, pv in enumerate(aais)])

    async def wait_dac_req() -> None:
        # TODO: Clear queue
        flag = aao_request.get()
        logger.debug(f"get dac request: {flag}")
        if not flag:
            async for flag in aao_request:
                logger.debug(f"monitor dac request: {flag}")
                if flag:
                    break

    async def run_check(config: Config) -> None:
        dac_mag = config.dac_max_abs_v
        attempts = 256
        timeout = 10.0

        async def check_attempts(check: Callable[[], Awaitable[None]]) -> None:
            bar = CountBar(total_count=attempts)
            bar.print()
            for i in range(attempts):
                await asyncio.wait_for(check(), timeout)
                bar.current_count = i + 1
                bar.print()
            bar.print()
            print()

        logger.info("Set one-shot DAC playback mode")
        #await aao_mode.put(False)

        logger.info("Check full-size DAC waveform")

        async def check_full() -> None:
            logger.debug("check_full")
            await wait_dac_req()
            await write_and_check_dac(dac_mag * np.arange(-wf_size, wf_size, 2, dtype=np.float64) / wf_size)

        await check_attempts(check_full)

        logger.info("Check two half-size DAC waveforms")

        async def check_half() -> None:
            await wait_dac_req()
            await write_and_check_dac(dac_mag * np.arange(-wf_size, 0, 2, dtype=np.float64) / wf_size)
            await wait_dac_req()
            await write_and_check_dac(dac_mag * np.arange(0, wf_size, 2, dtype=np.float64) / wf_size)

        await check_attempts(check_half)

        logger.info("Flush DAC and check all ADCs")
        await wait_dac_req()
        # Flush FakeDev chunk buffer
        await write_and_check_dac(np.zeros(FakeDev.request_size, dtype=np.float64))
        await asyncio.sleep(0.5)
        # Check total ADCs samples count
        assert all([sc == 2 * wf_size * attempts for sc in adcs_samples_count])

    await with_background(run_check(config), watch_adcs())


def run(source_dir: Path, epics_base_dir: Path, ioc_dir: Path, arch: str, env: Dict[str, str]) -> None:
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    os.environ.update(ca.local_env())

    ioc = AsyncIoc(epics_base_dir, ioc_dir, arch, env=env)
    repeater = ca.Repeater(epics_base_dir, arch)

    config = read_common_config(source_dir)
    handler = Handler(config)
    device = FakeDev(ioc, config, handler)

    with repeater:
        loop.run_until_complete(device.run_with(test(device.config, handler)))
