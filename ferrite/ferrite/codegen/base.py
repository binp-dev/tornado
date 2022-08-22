from __future__ import annotations
from typing import Any, List, Optional, Sequence, Set, Union

from dataclasses import dataclass
from enum import Enum
from random import Random

from numpy.typing import DTypeLike

from ferrite.codegen.utils import indent


@dataclass
class Context:
    prefix: Optional[str] = None
    iter_depth: int = 0


# FIXME: Remove global context
CONTEXT = Context()


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
    NONE = 0
    INCLUDES = 1
    DECLARATION = 2
    DEFINITION = 3


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


class Include(Source):

    def __init__(self, path: str):
        super().__init__(Location.INCLUDES, [[f"#include <{path}>"]])


def declare_variable(c_type: str, variable: str) -> str:
    return f"{c_type} {variable}"


class Type:

    def __init__(self, sized: bool, trivial: bool = False):
        if trivial:
            assert sized
        self.sized = sized
        self.trivial = trivial

    def name(self) -> Name:
        raise NotImplementedError(type(self).__name__)

    def _debug_name(self) -> str:
        try:
            return self.name().camel()
        except NotImplementedError:
            return type(self).__name__

    def _not_implemented(self) -> RuntimeError:
        return NotImplementedError(self._debug_name())

    def size(self) -> int:
        raise self._not_implemented()

    def min_size(self) -> int:
        return self.size()

    def is_empty(self) -> bool:
        return self.sized and self.size() == 0

    def deps(self) -> List[Type]:
        return []

    def load(self, data: bytes) -> Any:
        raise self._not_implemented()

    def store(self, value: Any) -> bytes:
        raise self._not_implemented()

    def default(self) -> Any:
        raise self._not_implemented()

    def random(self, rng: Random) -> Any:
        raise self._not_implemented()

    def is_instance(self, value: Any) -> bool:
        raise self._not_implemented()

    def __instancecheck__(self, value: Any) -> bool:
        return self.is_instance(value)

    def np_dtype(self) -> DTypeLike:
        raise self._not_implemented()

    def c_type(self) -> str:
        raise self._not_implemented()

    def rust_type(self) -> str:
        raise self._not_implemented()

    def pyi_type(self) -> str:
        raise self._not_implemented()

    def pyi_np_dtype(self) -> str:
        raise self._not_implemented()

    def c_source(self) -> Optional[Source]:
        return None

    def rust_source(self) -> Optional[Source]:
        return None

    def pyi_source(self) -> Optional[Source]:
        return None

    def c_size(self, obj: str) -> str:
        return str(self.size())

    def rust_size(self, obj: str) -> str:
        if self.sized:
            return f"<{self.rust_type()} as FlatSized>::SIZE"
        else:
            return f"<{self.rust_type()} as FlatBase>::size({obj})"

    def _c_size_extent(self, obj: str) -> str:
        raise self._not_implemented()

    def c_object(self, value: Any) -> str:
        raise self._not_implemented()

    def rust_object(self, value: Any) -> str:
        raise self._not_implemented()
