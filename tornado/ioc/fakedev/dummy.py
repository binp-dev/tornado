from __future__ import annotations
from typing import List

from pathlib import Path
from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

import asyncio

from ferrite.utils.epics.ioc import make_ioc

from tornado.common.config import read_common_config
from tornado.ioc.fakedev.base import FakeDev


@dataclass
class Handler(FakeDev.Handler):
    time: float = 0.0

    async def transfer(self, dac: NDArray[np.float64]) -> NDArray[np.float64]:
        adc_mag = dac / self.config.dac_max_abs_v * self.config.adc_max_abs_v
        time = self.time + np.arange(len(dac), dtype=np.float64) / self.config.sample_freq_hz
        value = 0.5 * adc_mag * np.cos(np.e * time) + 0.5 * self.config.adc_max_abs_v * np.cos(np.pi * time)

        delay = len(dac) / self.config.sample_freq_hz
        await asyncio.sleep(delay)
        self.time += delay

        return np.stack([dac] + [value] * (self.config.adc_count - 1)).transpose()


def run(source_dir: Path, epics_base_dir: Path, ioc_dir: Path, arch: str) -> None:

    ioc = make_ioc(ioc_dir, arch)

    config = read_common_config(source_dir)
    handler = Handler(config)
    device = FakeDev(ioc, config, handler)

    asyncio.run(device.run(), debug=True)
