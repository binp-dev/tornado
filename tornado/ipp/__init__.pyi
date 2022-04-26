from __future__ import annotations

from pathlib import Path


def generate(path: Path) -> None:
    ...


from dataclasses import dataclass
import numpy as np
from numpy.typing import NDArray


@dataclass
class AppMsgConnect:

    @staticmethod
    def load(data: bytes) -> AppMsgConnect:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsgKeepAlive:

    @staticmethod
    def load(data: bytes) -> AppMsgKeepAlive:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsgDoutUpdate:

    value: int

    @staticmethod
    def load(data: bytes) -> AppMsgDoutUpdate:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsgDacMode:

    enable: int

    @staticmethod
    def load(data: bytes) -> AppMsgDacMode:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsgDacData:

    points: NDArray[np.int32]

    @staticmethod
    def load(data: bytes) -> AppMsgDacData:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsgStatsReset:

    @staticmethod
    def load(data: bytes) -> AppMsgStatsReset:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsg:

    Connect = AppMsgConnect
    KeepAlive = AppMsgKeepAlive
    DoutUpdate = AppMsgDoutUpdate
    DacMode = AppMsgDacMode
    DacData = AppMsgDacData
    StatsReset = AppMsgStatsReset

    Variant = AppMsgConnect | AppMsgKeepAlive | AppMsgDoutUpdate | AppMsgDacMode | AppMsgDacData | AppMsgStatsReset

    variant: Variant

    @staticmethod
    def load(data: bytes) -> AppMsg:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class McuMsgDinUpdate:

    value: int

    @staticmethod
    def load(data: bytes) -> McuMsgDinUpdate:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class McuMsgDacRequest:

    count: int

    @staticmethod
    def load(data: bytes) -> McuMsgDacRequest:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class McuMsgAdcData:

    points_arrays: NDArray[np.int32]

    @staticmethod
    def load(data: bytes) -> McuMsgAdcData:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class McuMsgError:

    code: int
    message: str

    @staticmethod
    def load(data: bytes) -> McuMsgError:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class McuMsgDebug:

    message: str

    @staticmethod
    def load(data: bytes) -> McuMsgDebug:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class McuMsg:

    DinUpdate = McuMsgDinUpdate
    DacRequest = McuMsgDacRequest
    AdcData = McuMsgAdcData
    Error = McuMsgError
    Debug = McuMsgDebug

    Variant = McuMsgDinUpdate | McuMsgDacRequest | McuMsgAdcData | McuMsgError | McuMsgDebug

    variant: Variant

    @staticmethod
    def load(data: bytes) -> McuMsg:
        ...

    def store(self) -> bytes:
        ...
