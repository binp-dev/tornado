# This file was generatered by Ferrite Protogen.
from __future__ import annotations

from ferrite.protogen.base import Value

from dataclasses import dataclass
import numpy as np
from numpy.typing import NDArray


@dataclass
class Ai(Value):

    value: int


@dataclass
class Aai(Value):

    value: NDArray[np.int32]


@dataclass
class InMsg(Value):

    Variant = Ai | Aai

    Ai = Ai
    Aai = Aai

    variant: Variant


@dataclass
class Ao(Value):

    value: int


@dataclass
class Aao(Value):

    value: NDArray[np.int32]


@dataclass
class OutMsg(Value):

    Variant = Ao | Aao

    Ao = Ao
    Aao = Aao

    variant: Variant
