from __future__ import annotations
from typing import List

import math
import time
from pathlib import Path

from ferrite.utils.epics.ioc import make_ioc
import ferrite.utils.epics.ca as ca
from ferrite.ioc.fakedev.base import FakeDev


def assert_eq(a: float, b: float, eps: float = 1e-3) -> None:
    if abs(a - b) > eps:
        raise AssertionError(f"abs({a} - {b}) < {eps}")


class Handler(FakeDev.Handler):

    def __init__(self) -> None:
        self.mag = 0.0
        self.time = 0.0

    def step(self, dt: float) -> None:
        self.time += dt

    def write_dac(self, voltage: float) -> None:
        self.mag = 0.5 * voltage

    def read_adcs(self) -> List[float]:
        value = self.mag * math.cos(0.2718 * self.time) + 5.0 * math.cos(0.3141 * self.time)
        return [value] * 6


def run(epics_base_dir: Path, ioc_dir: Path, arch: str) -> None:

    prefix = epics_base_dir / "bin" / arch
    ioc = make_ioc(ioc_dir, arch)
    handler = Handler()

    with FakeDev(prefix, ioc, handler):
        while True:
            delay = 0.1
            time.sleep(delay)
            handler.step(delay)
