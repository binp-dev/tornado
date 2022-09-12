# This file was generatered by Ferrite Protogen.
from __future__ import annotations

from ferrite.protogen.base import Value

from dataclasses import dataclass
from typing import List


@dataclass
class Ai(Value):

    value: int


@dataclass
class Aai(Value):

    value: List[int]


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

    value: List[int]


@dataclass
class OutMsg(Value):

    Variant = Ao | Aao

    Ao = Ao
    Aao = Aao

    variant: Variant
