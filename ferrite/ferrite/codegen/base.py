from __future__ import annotations
from typing import Any, Generic, List, Optional, Sequence, Set, TypeVar, Union

from dataclasses import dataclass
from enum import Enum
from random import Random

from ferrite.codegen.utils import indent_text


@dataclass
class Context:
    prefix: Optional[str] = None
    test_attempts: int = 1


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


class Location(Enum):
    NONE = 0
    INCLUDES = 1
    DECLARATION = 2
    DEFINITION = 3
    TESTS = 4


class Source:

    def __init__(
        self,
        location: Location,
        items: Union[Sequence[Optional[str]], str, None] = None,
        deps: List[Optional[Source]] = [],
    ):
        self.location = location

        if items is not None:
            if isinstance(items, str):
                self.items = [items]
            elif isinstance(items, list):
                self.items = [it for it in items if it is not None]
            else:
                raise RuntimeError(f"Unsupported type {type(items).__name__}")
        else:
            self.items = []

        self.deps = [p for p in deps if p is not None]

    def collect(self, location: Location, used: Optional[Set[str]] = None) -> List[str]:
        if used is None:
            used = set()

        result = []

        for dep in self.deps:
            result.extend(dep.collect(location, used))

        if self.location == location:
            for item in self.items:
                if item is not None and item not in used:
                    result.append(item)
                    used.add(item)

        return result

    def make_source(self, location: Location, separator: str = "\n\n") -> str:
        return separator.join(self.collect(location)) + "\n"


class Include(Source):

    def __init__(self, path: str):
        super().__init__(Location.INCLUDES, [f"#include <{path}>"])


def declare_variable(c_type: str, variable: str) -> str:
    return f"{c_type} {variable}"


T = TypeVar("T")


class Type(Generic[T]):

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

    def size(self) -> int:
        raise NotImplementedError(self._debug_name())

    def min_size(self) -> int:
        return self.size()

    def is_empty(self) -> bool:
        return self.sized and self.size() == 0

    def deps(self) -> List[Type[Any]]:
        return []

    def load(self, data: bytes) -> T:
        raise NotImplementedError(self._debug_name())

    def store(self, value: T) -> bytes:
        raise NotImplementedError(self._debug_name())

    def random(self, rng: Random) -> T:
        raise NotImplementedError(self._debug_name())

    def is_instance(self, value: T) -> bool:
        raise NotImplementedError(self._debug_name())

    def __instancecheck__(self, value: T) -> bool:
        return self.is_instance(value)

    def c_type(self) -> str:
        raise NotImplementedError(self._debug_name())

    def cpp_type(self) -> str:
        return self.c_type()

    def pyi_type(self) -> str:
        raise NotImplementedError(self._debug_name())

    def c_source(self) -> Optional[Source]:
        return None

    def cpp_source(self) -> Optional[Source]:
        return self.c_source()

    def pyi_source(self) -> Optional[Source]:
        return None

    def c_size(self, obj: str) -> str:
        return str(self.size())

    def cpp_size(self, obj: str) -> str:
        return self.c_size(obj)

    def _c_size_extent(self, obj: str) -> str:
        raise NotImplementedError(self._debug_name())

    def _cpp_size_extent(self, obj: str) -> str:
        raise NotImplementedError(self._debug_name())

    def cpp_load(self, src: str) -> str:
        if self.trivial:
            return f"{src}"
        raise NotImplementedError(self._debug_name())

    def cpp_store(self, src: str, dst: str) -> str:
        if self.trivial:
            return f"{dst} = {src}"
        raise NotImplementedError(self._debug_name())

    def cpp_object(self, value: T) -> str:
        raise NotImplementedError(self._debug_name())

    def c_test(self, obj: str, src: str) -> str:
        return self.cpp_test(self.cpp_load(obj), src)

    def cpp_test(self, dst: str, src: str) -> str:
        return f"EXPECT_EQ({dst}, {src});"

    def _cpp_static_check(self) -> Optional[str]:
        return f"static_assert(sizeof({self.c_type()}) == size_t({self.size() if self.sized else self.min_size()}));"

    def test_source(self) -> Optional[Source]:
        if self.trivial:
            return None

        rng = Random(0xdeadbeef)
        static_check = self._cpp_static_check()
        return Source(
            Location.TESTS,
            "\n".join([
                f"TEST({Name(CONTEXT.prefix, 'test').camel()}, {self.name().camel()}) {{",
                indent_text(
                    "\n".join([
                        *([
                            static_check,
                            f"",
                        ] if static_check is not None else []),
                        f"std::vector<{self.cpp_type()}> srcs = {{",
                        *[indent_text(self.cpp_object(self.random(rng)), "    ") + "," for _ in range(CONTEXT.test_attempts)],
                        f"}};",
                        f"",
                        f"for (size_t k = 0; k < {CONTEXT.test_attempts}; ++k) {{",
                        indent_text(
                            "\n".join([
                                f"const {self.cpp_type()} src = srcs[k];",
                                f"",
                                f"std::vector<uint8_t> buf({self.cpp_size('src')});",
                                f"auto *obj = reinterpret_cast<{self.c_type()} *>(buf.data());",
                                f"{self.cpp_store('src', '(*obj)')};",
                                f"ASSERT_EQ({self.c_size('(*obj)')}, {self.cpp_size('src')});",
                                f"{self.c_test('(*obj)', 'src')}",
                                f"",
                                f"const auto dst = {self.cpp_load('(*obj)')};",
                                f"ASSERT_EQ({self.cpp_size('dst')}, {self.cpp_size('src')});",
                                f"{self.cpp_test('dst', 'src')}",
                            ]),
                            "    ",
                        ),
                        f"}}",
                    ]),
                    "    ",
                ),
                f"}}",
            ]),
            deps=[ty.test_source() for ty in self.deps()],
        )
