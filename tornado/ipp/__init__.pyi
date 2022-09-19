# This file was generatered by Ferrite Protogen.
from __future__ import annotations

from ferrite.protogen.base import Value

from dataclasses import dataclass
import numpy as np
from numpy.typing import NDArray


@dataclass
class AppMsgEmpty(Value):

    pass


@dataclass
class AppMsgConnect(Value):

    pass


@dataclass
class AppMsgKeepAlive(Value):

    pass


@dataclass
class AppMsgDoutUpdate(Value):

    value: int


@dataclass
class AppMsgDacMode(Value):

    enable: int


@dataclass
class AppMsgDacData(Value):

    points: NDArray[np.int32]


@dataclass
class AppMsgStatsReset(Value):

    pass


@dataclass
class AppMsg(Value):

    Variant = AppMsgEmpty | AppMsgConnect | AppMsgKeepAlive | AppMsgDoutUpdate | AppMsgDacMode | AppMsgDacData | AppMsgStatsReset

    Empty = AppMsgEmpty
    Connect = AppMsgConnect
    KeepAlive = AppMsgKeepAlive
    DoutUpdate = AppMsgDoutUpdate
    DacMode = AppMsgDacMode
    DacData = AppMsgDacData
    StatsReset = AppMsgStatsReset

    variant: Variant


@dataclass
class McuMsgEmpty(Value):

    pass


@dataclass
class McuMsgDinUpdate(Value):

    value: int


@dataclass
class McuMsgDacRequest(Value):

    count: int


@dataclass
class McuMsgAdcData(Value):

    points_arrays: NDArray[np.int32]


@dataclass
class McuMsgError(Value):

    code: int
    message: str


@dataclass
class McuMsgDebug(Value):

    message: str


@dataclass
class McuMsg(Value):

    Variant = McuMsgEmpty | McuMsgDinUpdate | McuMsgDacRequest | McuMsgAdcData | McuMsgError | McuMsgDebug

    Empty = McuMsgEmpty
    DinUpdate = McuMsgDinUpdate
    DacRequest = McuMsgDacRequest
    AdcData = McuMsgAdcData
    Error = McuMsgError
    Debug = McuMsgDebug

    variant: Variant
