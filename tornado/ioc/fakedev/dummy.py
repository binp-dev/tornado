from __future__ import annotations
from typing import List

import math
from pathlib import Path
from dataclasses import dataclass

import asyncio

from ferrite.utils.epics.ioc import make_ioc

from tornado.common.config import read_common_config
from tornado.ioc.fakedev.base import FakeDev


@dataclass
class Handler(FakeDev.Handler):
    time: float = 0.0

    def transfer(self, dac: float) -> List[float]:
        adc_mag = dac / self.config.dac_max_abs_v * self.config.adc_max_abs_v
        value = 0.5 * adc_mag * math.cos(math.e * self.time) + 0.5 * self.config.adc_max_abs_v * math.cos(math.pi * self.time)
        self.time += 1.0 / self.config.sample_freq_hz
        return [dac] + [value] * (self.config.adc_count - 1)


def run(source_dir: Path, ioc_dir: Path, arch: str) -> None:

    ioc = make_ioc(ioc_dir, arch)

    config = read_common_config(source_dir)
    handler = Handler(config)
    device = FakeDev(ioc, config, handler)

    asyncio.run(device.run(), debug=True)
