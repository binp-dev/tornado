from __future__ import annotations
from typing import Dict

from pathlib import Path
from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

import asyncio

from ferrite.utils.epics.ioc import AsyncIoc

from tornado.fakedev.base import FakeDev
from tornado import config


@dataclass
class Handler(FakeDev.Handler):
    time: float = 0.0

    async def transfer(self, dac: NDArray[np.float64]) -> NDArray[np.float64]:
        adc_mag = dac / config.DAC_MAX_ABS_V * config.ADC_MAX_ABS_V
        time = self.time + np.arange(len(dac), dtype=np.float64) / config.SAMPLE_FREQ_HZ
        value = 0.5 * adc_mag * np.cos(np.e * time) + 0.5 * config.ADC_MAX_ABS_V * np.cos(np.pi * time)

        delay = len(dac) / config.SAMPLE_FREQ_HZ
        await asyncio.sleep(delay)
        self.time += delay

        return np.stack([dac] + [value] * (config.ADC_COUNT - 1)).transpose()


def run(source_dir: Path, epics_base_dir: Path, ioc_dir: Path, arch: str, env: Dict[str, str]) -> None:

    ioc = AsyncIoc(epics_base_dir, ioc_dir, arch, env=env)

    handler = Handler()
    device = FakeDev(ioc, handler)

    asyncio.run(device.run(), debug=True)
