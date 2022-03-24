from __future__ import annotations
from typing import Dict, List

import math
from pathlib import Path
from dataclasses import dataclass
import asyncio

from ferrite.utils.asyncio import with_background
from ferrite.utils.epics.ioc import make_ioc
from ferrite.utils.epics.asyncio import Context, Pv, PvType

from tornado.common.config import read_common_config
from tornado.ioc.fakedev.base import FakeDev

import logging

logger = logging.getLogger(__name__)


def approx_eq(a: float, b: float, eps: float = 1e-3) -> bool:
    return abs(a - b) <= eps


@dataclass
class Handler(FakeDev.Handler):
    _dt: float = 0.0

    class _Watcher:

        def __init__(self, count: int) -> None:
            self.count = count
            self.event = asyncio.Event()

    def __post_init__(self) -> None:
        # Waveform layout in list: [ADC0, ..., ADC{N-1}, DAC]
        self.wfs: List[List[float]] = [[] for _ in range(self.config.adc_count + 1)]
        self.watchers: Dict[int, Handler._Watcher] = {}

    def transfer(self, dac: float) -> List[float]:
        dac_wf = self.wfs[-1]
        adc_wfs = self.wfs[:-1]

        dac_wf.append(dac)

        adcs = [0.0] * self.config.adc_count
        adcs[0] = dac / self.config.dac_max_abs_v * self.config.adc_max_abs_v
        for i in range(1, self.config.adc_count):
            adcs[i] = self.config.adc_max_abs_v * math.sin(2.0 * math.pi * i * self._dt)
        self._dt += 1e-4

        for x, wf in zip(adcs, adc_wfs):
            wf.append(x)

        # Notify watchers
        for idx, watcher in self.watchers.items():
            if len(self.wfs[idx]) >= watcher.count:
                watcher.event.set()

        return adcs

    @staticmethod
    def _assert_wfs_eq(a_wf: List[float], b_wf: List[float]) -> None:
        assert len(a_wf) == len(b_wf)
        assert all((approx_eq(a, b) for a, b in zip(a_wf, b_wf)))

    async def _check_wf(self, idx: int, subwf: List[float]) -> None:
        size = len(subwf)

        if len(self.wfs[idx]) < size:
            # Waveform is not ready, waiting
            assert idx not in self.watchers.keys(), f"Waveform[{idx}] checking is already in progress"
            watcher = Handler._Watcher(size)
            self.watchers[idx] = watcher
            await watcher.event.wait()
            del self.watchers[idx]
            assert len(self.wfs[idx]) >= size

        Handler._assert_wfs_eq(subwf, self.wfs[idx][:size])
        self.wfs[idx] = self.wfs[idx][size:]

    async def check_dac(self, dac_subwf: List[float]) -> None:
        await self._check_wf(-1, dac_subwf)

    async def check_adc(self, index: int, adc_subwf: List[float]) -> None:
        assert index in range(0, self.config.adc_count)
        await self._check_wf(index, adc_subwf)


async def async_run(config: FakeDev.Config, handler: Handler) -> None:
    ctx = Context()
    aais = [await ctx.connect(f"aai{i}", PvType.ARRAY_FLOAT) for i in range(config.common.adc_count)]
    aao = await ctx.connect("aao0", PvType.ARRAY_FLOAT)
    aao_request = await ctx.connect("aao0_request", PvType.BOOL)
    aao_cyclic = await ctx.connect("aao0_cyclic", PvType.BOOL)

    wf_size = aao.nelm
    logger.debug(f"Waveform max size: {wf_size}")
    # Check that `aai*` sizes are the same as `aao0` size
    assert all([wf_size == aai.nelm for aai in aais])

    async def write_and_check_dac(array: List[float]) -> None:
        await aao.put(array)
        await handler.check_dac(array)
        logger.debug(f"DAC of size {len(array)} is correct")

    adcs_samples_count = [0] * config.common.adc_count

    async def watch_single_adc(index: int, adc_pv: Pv[List[float]]) -> None:
        async with adc_pv.monitor() as mon:
            async for array in mon:
                if len(array) == 0:
                    continue
                await handler.check_adc(index, array)
                adcs_samples_count[index] += len(array)
                logger.debug(f"ADC[{index}] of size {len(array)} is correct")

    async def watch_adcs() -> None:
        await asyncio.gather(*[watch_single_adc(i, pv) for i, pv in enumerate(aais)])

    async def wait_dac_req() -> None:
        async with aao_request.monitor(current=True) as mon:
            async for flag in mon:
                if int(flag) != 0:
                    break

    async def run_check(config: FakeDev.Config) -> None:
        dac_mag = config.common.dac_max_abs_v

        logger.info("Set one-shot DAC playback mode")
        await aao_cyclic.put(False)

        logger.info("Check full-size DAC waveform")
        await wait_dac_req()
        await write_and_check_dac([dac_mag * x / wf_size for x in range(wf_size)])

        logger.info("Check two half-size DAC waveforms")
        await wait_dac_req()
        await write_and_check_dac([dac_mag * x / wf_size for x in range(0, -wf_size, -2)])
        await wait_dac_req()
        await write_and_check_dac([dac_mag * x / wf_size for x in range(-wf_size, 0, 2)])

        logger.info("Flush DAC and check all ADCs")
        await wait_dac_req()
        # Flush FakeDev chunk buffer
        await write_and_check_dac([0.0] * config.adc_chunk_size)
        await asyncio.sleep(0.5)
        # Check total ADCs samples count
        assert all([sc >= 2 * wf_size for sc in adcs_samples_count])

    await with_background(run_check(config), watch_adcs())


def run(source_dir: Path, ioc_dir: Path, arch: str) -> None:
    ioc = make_ioc(ioc_dir, arch)

    config = read_common_config(source_dir)
    handler = Handler(config)
    device = FakeDev(ioc, config, handler)

    asyncio.run(with_background(
        async_run(device.config, handler),
        device.run(),
    ))
