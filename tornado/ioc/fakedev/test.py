from __future__ import annotations
from typing import Dict, List

from pathlib import Path
from dataclasses import dataclass
import asyncio

from ferrite.utils.epics.ioc import make_ioc
from ferrite.utils.epics.asyncio import Context, Pv, PvType

from tornado.ioc.fakedev.base import FakeDev

import logging

logger = logging.getLogger(__name__)


def assert_eq(a: float, b: float, eps: float = 1e-3) -> None:
    if abs(a - b) > eps:
        raise AssertionError(f"abs({a} - {b}) < {eps}")


@dataclass
class Handler(FakeDev.Handler):
    config: FakeDev.Config

    class _Watcher:

        def __init__(self, count: int) -> None:
            self.count = count
            self.event = asyncio.Event()

    def __post_init__(self) -> None:
        # Waveform layout in list: [ADC0, ..., ADC{N-1}, DAC]
        self.wfs: List[List[int]] = [[] for _ in range(self.config.adc_count + 1)]
        self.watchers: Dict[int, Handler._Watcher] = {}

    # FIXME: Use volts
    def transfer_codes(self, dac_code: int) -> List[int]:
        dac_wf = self.wfs[-1]
        adc_wfs = self.wfs[:-1]

        dac_wf.append(dac_code)
        adc_codes = [10000 * i + dac_code for i in range(len(adc_wfs))]
        for x, wf in zip(adc_codes, adc_wfs):
            wf.append(x)

        # Notify watchers
        for idx, watcher in self.watchers.items():
            if len(self.wfs[idx]) >= watcher.count:
                watcher.event.set()

        return adc_codes

    @staticmethod
    def _eq_wfs(a_wf: List[int], b_wf: List[int]) -> bool:
        assert len(a_wf) == len(b_wf)
        return all((a == b for a, b in zip(a_wf, b_wf)))

    async def _check_wf(self, idx: int, subwf: List[int]) -> None:
        size = len(subwf)

        if len(self.wfs[idx]) < size:
            # Waveform is not ready, waiting
            assert idx not in self.watchers.keys(), f"Waveform[{idx}] checking is already in progress"
            watcher = Handler._Watcher(size)
            self.watchers[idx] = watcher
            await watcher.event.wait()
            del self.watchers[idx]
            assert len(self.wfs[idx]) >= size

        assert Handler._eq_wfs(subwf, self.wfs[idx][:size])
        self.wfs[idx] = self.wfs[idx][size:]

    async def check_dac(self, dac_subwf: List[int]) -> None:
        await self._check_wf(-1, dac_subwf)

    async def check_adc(self, index: int, adc_subwf: List[int]) -> None:
        assert index in range(0, self.config.adc_count)
        await self._check_wf(index, adc_subwf)


async def async_run(config: FakeDev.Config, device: FakeDev, handler: Handler) -> None:
    dev_task = asyncio.create_task(device.run())

    ctx = Context()
    aais = [await ctx.pv(f"aai{i}", PvType.ARRAY_INT) for i in range(config.adc_count)]
    aao = await ctx.pv("aao0", PvType.ARRAY_INT)
    aao_req = await ctx.pv("aao0_req", PvType.BOOL)
    aao_cyclic = await ctx.pv("aao0_cyclic", PvType.BOOL)

    wf_size = await (await ctx.pv("aao0.NELM", PvType.INT)).read()
    logger.debug(f"Waveform max size: {wf_size}")
    # Check that `aai*` sizes are the same as `aao0` size
    assert all([wf_size == await (await ctx.pv(f"aai{i}.NELM", PvType.INT)).read() for i in range(len(aais))])

    async def write_and_check_dac(array: List[int]) -> None:
        await aao.write(array)
        await handler.check_dac(array)
        logger.debug(f"DAC of size {len(array)} is correct")

    adcs_samples_count = [0] * config.adc_count

    async def watch_single_adc(index: int, adc_pv: Pv[List[int]]) -> None:
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
        async with aao_req.monitor() as mon:
            async for flag in mon:
                if int(flag) != 0:
                    break

    watch_task = asyncio.create_task(watch_adcs())

    logger.info("Set one-shot DAC playback mode")
    await aao_cyclic.write(False)

    logger.info("Check empty DAC waveform")
    await wait_dac_req()
    await write_and_check_dac([])

    logger.info("Check full-size DAC waveform")
    await wait_dac_req()
    await write_and_check_dac(list(range(wf_size)))

    logger.info("Check two half-size DAC waveforms")
    await wait_dac_req()
    await write_and_check_dac(list(range(0, -wf_size // 2, -1)))
    await wait_dac_req()
    await write_and_check_dac(list(range(-wf_size // 2, 0)))

    logger.info("Flush DAC and check all ADCs")
    await wait_dac_req()
    # Flush FakeDev chunk buffer
    await write_and_check_dac(list(range(config.chunk_size)))
    await asyncio.sleep(0.5)
    # Check total ADCs samples count
    assert all([sc >= 2 * wf_size for sc in adcs_samples_count])

    watch_task.cancel()
    dev_task.cancel()


def run(source_dir: Path, ioc_dir: Path, arch: str) -> None:
    ioc = make_ioc(ioc_dir, arch)

    config = FakeDev.read_config(source_dir)
    handler = Handler(config)
    device = FakeDev(ioc, config, handler)

    # Run with timeout
    asyncio.run(asyncio.wait_for(async_run(config, device, handler), 60.0))
