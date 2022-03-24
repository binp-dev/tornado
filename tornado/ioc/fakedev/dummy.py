from __future__ import annotations
from typing import List

import math
from pathlib import Path
from dataclasses import dataclass

import asyncio

from ferrite.utils.asyncio import with_background
from ferrite.utils.epics.ioc import make_ioc
from ferrite.utils.epics.asyncio import Context, PvType

from tornado.ioc.fakedev.base import FakeDev


@dataclass
class Handler(FakeDev.Handler):
    config: FakeDev.Config
    time: float = 0.0

    def transfer(self, dac: float) -> List[float]:
        value = 0.5 * dac * math.cos(math.e * self.time) + 5.0 * math.cos(math.pi * self.time)
        self.time += self.config.sample_period
        return [dac] + [value] * (self.config.adc_count - 1)


async def test() -> None:
    ctx = Context()
    ai = await ctx.connect("ai0", PvType.FLOAT)
    aao = await ctx.connect("aao0", PvType.ARRAY_INT)

    async def put() -> None:
        aao.put([32000 * i // aao.nelm for i in range(aao.nelm)])

    async def monitor() -> None:
        async with ai.monitor() as m:
            async for v in m:
                print(f"ai0: {v}")

    await asyncio.gather(put(), monitor())


def run(source_dir: Path, ioc_dir: Path, arch: str) -> None:

    ioc = make_ioc(ioc_dir, arch)

    config = FakeDev.read_config(source_dir)
    handler = Handler(config)
    device = FakeDev(ioc, config, handler)

    asyncio.run(with_background(test(), device.run()), debug=True)
