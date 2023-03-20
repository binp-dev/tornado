from __future__ import annotations
from typing import Any, List, Optional, Sequence, Set, Union

from dataclasses import dataclass
from enum import Enum


@dataclass
class TestInfo:
    rng_seed = 0xdeadbeef
    attempts: int = 4


@dataclass
class Context:
    prefix: str
    portable: bool = False

    def set_global(self) -> None:
        global CONTEXT
        for k in dir(self):
            if not k.startswith("__"):
                setattr(CONTEXT, k, getattr(self, k))


# FIXME: Remove global context
CONTEXT = Context("default")


class Name:

    def __init__(self, *args: Union[str, List[str], Name, None]):
        self.words: List[str] = []
        for arg in args:
            if isinstance(arg, Name):
                self.words += arg.words
            elif isinstance(arg, list):
                self.words += arg
            elif isinstance(arg, str):
                self.words.append(arg)
            elif arg is None:
                pass
            else:
                raise RuntimeError(f"Unsupported argument {type(arg).__name__}")

    def camel(self) -> str:
        return "".join([s[0].upper() + s[1:].lower() for s in self.words])

    def snake(self) -> str:
        return "_".join([s.lower() for s in self.words])

    @staticmethod
    def from_snake(snake: str) -> Name:
        return Name(snake.split("_"))

    @staticmethod
    def from_camel(camel: str) -> Name:
        parts = [camel[0].lower()]
        for c in camel[1:]:
            cl = c.lower()
            if c != cl:
                parts.append(cl)
            else:
                parts[-1] += c
        return Name(parts)

    def __hash__(self) -> int:
        return hash(tuple(self.words))

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Name):
            raise NotImplementedError()
        return self.words == other.words

    def __ne__(self, other: object) -> bool:
        if not isinstance(other, Name):
            raise NotImplementedError()
        return not (self == other)

    def __repr__(self) -> str:
        return f"[" + ", ".join(self.words) + "]"


class Location(Enum):
    IMPORT = 0
    DECLARATION = 1
    DEFINITION = 2
    TEST = 3


class Source:

    def __init__(
        self,
        location: Location,
        items: Sequence[List[str]] = [],
        deps: List[Optional[Source]] = [],
    ):
        self.location = location
        self.items = ["\n".join(s) + "\n" for s in items]
        self.deps = [p for p in deps if p is not None]

    def collect(self, location: Location, used: Optional[Set[int]] = None) -> List[str]:
        if used is None:
            used = set()

        result = []

        for dep in self.deps:
            result.extend(dep.collect(location, used))

        if self.location == location:
            for item in self.items:
                item_hash = hash(item)
                if item_hash not in used:
                    result.append(item)
                    used.add(item_hash)

        return result

    def make_source(self, location: Location, separator: str = "\n") -> str:
        return separator.join(self.collect(location))


class UnexpectedEof(RuntimeError):

    def __init__(self, type: Any, data: bytes) -> None:
        super().__init__(f"Unexpected EOF while loading {type._debug_name()} from {data!r}")
