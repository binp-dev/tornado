from __future__ import annotations

from dataclasses import dataclass
from typing import List

from pathlib import Path


def generate(path: Path) -> None:
    ...


@dataclass
class AppMsgStart:

    @staticmethod
    def load(data: bytes) -> AppMsgStart:
        ...

    def store(self) -> bytes:
        ...


@dataclass
class AppMsgStop:

    @staticmethod
    def load(data: bytes) -> AppMsgStop:
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

    Start = AppMsgStart
    Stop = AppMsgStop
    DoutSet = AppMsgDoutSet
    DacWf = AppMsgDacWf

    Variant = AppMsgStart | AppMsgStop | AppMsgDoutSet | AppMsgDacWf

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
class McuMsgDacWfReq:

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
class McuMsg:

    DinVal = McuMsgDinVal
    Error = McuMsgError
    Debug = McuMsgDebug
    DacWfReq = McuMsgDacWfReq
    AdcWf = McuMsgAdcWf

    Variant = McuMsgDinVal | McuMsgError | McuMsgDebug | McuMsgDacWfReq | McuMsgAdcWf

    variant: Variant

    @staticmethod
    def load(data: bytes) -> McuMsg:
        ...

    def store(self) -> bytes:
        ...
