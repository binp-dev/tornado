# This file was generatered by Ferrite Protogen.
from __future__ import annotations

from ferrite.codegen.base import Value

from dataclasses import dataclass
import numpy as np
from numpy.typing import NDArray


@dataclass
class Ai(Value):

    value: int


@dataclass
class Aai(Value):

    values: NDArray[np.int32]


@dataclass
class Waveform(Value):

    values: NDArray[np.int32]


@dataclass
class Bi(Value):

    value: int


@dataclass
class MbbiDirect(Value):

    value: int


@dataclass
class InMsg(Value):

    Variant = Ai | Aai | Waveform | Bi | MbbiDirect

    Ai = Ai
    Aai = Aai
    Waveform = Waveform
    Bi = Bi
    MbbiDirect = MbbiDirect

    variant: Variant


@dataclass
class Ao(Value):

    value: int


@dataclass
class Aao(Value):

    values: NDArray[np.int32]


@dataclass
class Bo(Value):

    value: int


@dataclass
class MbboDirect(Value):

    value: int


@dataclass
class OutMsg(Value):

    Variant = Ao | Aao | Bo | MbboDirect

    Ao = Ao
    Aao = Aao
    Bo = Bo
    MbboDirect = MbboDirect

    variant: Variant
