# This file was generatered by Ferrite Codegen.
from __future__ import annotations

from dataclasses import dataclass
from typing import List


@dataclass
class Ai:

    value: int

    @staticmethod
    def load(data: bytes) -> Ai:
        ...

    def store(self) -> bytes:
        ...

    def size(self) -> int:
        ...


@dataclass
class Aai:

    value: List[int]

    @staticmethod
    def load(data: bytes) -> Aai:
        ...

    def store(self) -> bytes:
        ...

    def size(self) -> int:
        ...


@dataclass
class InMsg:

    Variant = Ai | Aai

    Ai = Ai
    Aai = Aai

    variant: Variant

    @staticmethod
    def load(data: bytes) -> InMsg:
        ...

    def store(self) -> bytes:
        ...

    def size(self) -> int:
        ...


@dataclass
class Ao:

    value: int

    @staticmethod
    def load(data: bytes) -> Ao:
        ...

    def store(self) -> bytes:
        ...

    def size(self) -> int:
        ...


@dataclass
class Aao:

    value: List[int]

    @staticmethod
    def load(data: bytes) -> Aao:
        ...

    def store(self) -> bytes:
        ...

    def size(self) -> int:
        ...


@dataclass
class OutMsg:

    Variant = Ao | Aao

    Ao = Ao
    Aao = Aao

    variant: Variant

    @staticmethod
    def load(data: bytes) -> OutMsg:
        ...

    def store(self) -> bytes:
        ...

    def size(self) -> int:
        ...
