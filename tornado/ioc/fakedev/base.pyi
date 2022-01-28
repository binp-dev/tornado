# TODO: Remove this file on ipp typings fix and pyzmq update

from __future__ import annotations
from typing import Any, List

from pathlib import Path

from ferrite.utils.epics.ioc import Ioc
import ferrite.utils.epics.ca as ca


def dac_code_to_volt(code: int) -> float:
    ...


def adc_volt_to_code(voltage: float) -> int:
    ...


class FakeDev:

    class Handler:

        def write_dac(self, voltage: float) -> None:
            ...

        def read_adcs(self) -> List[float]:
            ...

        def write_dac_code(self, code: int) -> None:
            ...

        def read_adc_codes(self) -> List[int]:
            ...

    def __init__(self, prefix: Path, ioc: Ioc, handler: FakeDev.Handler) -> None:
        ...

    def __enter__(self) -> None:
        ...

    def __exit__(self, *args: Any) -> None:
        ...
