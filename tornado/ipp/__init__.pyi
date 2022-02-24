from __future__ import annotations

from dataclasses import dataclass
from typing import List

from pathlib import Path


def generate(path: Path) -> None:
    ...


@dataclass
class AppMsgConnect:

    @staticmethod
    def load(data: bytes) -> AppMsgConnect:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsgStartDac:

    @staticmethod
    def load(data: bytes) -> AppMsgStartDac:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsgStopDac:

    @staticmethod
    def load(data: bytes) -> AppMsgStopDac:
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
class AppMsgDoutSet:

    value: int

    @staticmethod
    def load(data: bytes) -> AppMsgDoutSet:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsgDacWf:

    elements: List[int]

    @staticmethod
    def load(data: bytes) -> AppMsgDacWf:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsg:

    Connect = AppMsgConnect
    StartDac = AppMsgStartDac
    StopDac = AppMsgStopDac
    KeepAlive = AppMsgKeepAlive
    DoutSet = AppMsgDoutSet
    DacWf = AppMsgDacWf

    Variant = AppMsgConnect | AppMsgStartDac | AppMsgStopDac | AppMsgKeepAlive | AppMsgDoutSet | AppMsgDacWf

    variant: Variant

    @staticmethod
    def load(data: bytes) -> AppMsg:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class McuMsgDinVal:

    value: int

    @staticmethod
    def load(data: bytes) -> McuMsgDinVal:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class McuMsgDacWfReq:

    count: int

    @staticmethod
    def load(data: bytes) -> McuMsgDacWfReq:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class McuMsgAdcWf:

    index: int
    elements: List[int]

    @staticmethod
    def load(data: bytes) -> McuMsgAdcWf:
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

    DinVal = McuMsgDinVal
    DacWfReq = McuMsgDacWfReq
    AdcWf = McuMsgAdcWf
    Error = McuMsgError
    Debug = McuMsgDebug

    Variant = McuMsgDinVal | McuMsgDacWfReq | McuMsgAdcWf | McuMsgError | McuMsgDebug

    variant: Variant

    @staticmethod
    def load(data: bytes) -> McuMsg:
        ...

    def store(self) -> bytes:
        ...
