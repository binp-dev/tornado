from __future__ import annotations
from typing import List

import math
from pathlib import Path
from dataclasses import dataclass
import asyncio

from ferrite.utils.epics.ioc import make_ioc

from tornado.ioc.fakedev.base import FakeDev


@dataclass
class Handler(FakeDev.Handler):
    config: FakeDev.Config
    time: float = 0.0

    def transfer(self, dac: float) -> List[float]:
        value = 0.5 * dac * math.cos(math.e * self.time) + 5.0 * math.cos(math.pi * self.time)
        self.time += self.config.sample_period
        return [dac] + [value] * (self.config.adc_count - 1)


def run(epics_base_dir: Path, ioc_dir: Path, arch: str) -> None:

    prefix = epics_base_dir / "bin" / arch
    ioc = make_ioc(ioc_dir, arch)

    config = FakeDev.default_config()
    handler = Handler(config)
    device = FakeDev(prefix, ioc, config, handler)

    asyncio.run(device.run(), debug=True)
