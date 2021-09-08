from __future__ import annotations
from typing import Any, List, Set, Tuple, Union
from dataclasses import dataclass
from enum import Enum
from random import Random

from codegen.util import indent_text

@dataclass
class Context:
    prefix: str = None
    test_attempts: int = 1

CONTEXT = Context()

class Name:
    def __init__(self, *args):
        self.words = []
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

    def camel(self):
        return "".join([s[0].upper() + s[1:].lower() for s in self.words])

    def snake(self):
        return "_".join([s.lower() for s in self.words])

    @staticmethod
    def from_snake(snake: str) -> Name:
        Name(snake.split("_"))

    def __eq__(self, other: Name) -> bool:
        return self.words == other.words

    def __ne__(self, other: Name) -> bool:
        return not (self == other)

class Location(Enum):
    INCLUDES = 0
    DECLARATION = 1
    DEFINITION = 2
    TESTS = 3

class Source:
    def __init__(self, location: Location, items: Union[List[str], str] = None, deps: List[Source] = []):
        self.location = location

        if items is not None:
            if isinstance(items, str):
                self.items = [items]
            elif isinstance(items, list):
                self.items = items
            else:
                raise RuntimeError(f"Unsupported type {type(items).__name__}")
        else:
            self.items = []

        self.deps = [p for p in deps if p is not None]

    def collect(self, location: Location, used: Set[str] = None) -> List[str]:
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
    def __init__(self, path):
        super().__init__(Location.INCLUDES, [f"#include <{path}>"])

class CType:
    def __init__(self, prefix: str, postfix: str = None):
        self.prefix = prefix
        self.postfix = postfix
    
    def __str__(self) -> str:
        if self.postfix is not None:
            raise RuntimeError("CType has a postfix")
        return self.prefix
    
    def declare(self, variable: str) -> str:
        return f"{self.prefix} {variable}{self.postfix or ''}"

def declare_variable(c_type: Union[CType, str], variable: str) -> str:
    if isinstance(c_type, CType):
        return c_type.declare(variable)
    else:
        return f"{c_type} {variable}"

class Type:
    def __init__(self, sized: bool = False, trivial: bool = False):
        self._sized = sized
        self._trivial = trivial

    @property
    def sized(self):
        return self._sized

    @property
    def trivial(self):
        return self._trivial

    def name(self) -> Name:
        raise NotImplementedError()

    def min_size(self) -> int:
        raise NotImplementedError()

    def is_empty(self) -> bool:
        return self.sized and self.size() == 0

    def deps(self) -> List[Type]:
        return []

    def load(self, data: bytes) -> Any:
        raise NotImplementedError()
    
    def store(self, value: Any) -> bytes:
        raise NotImplementedError()

    def value(self, *args, **kwargs) -> Any:
        raise NotImplementedError()

    def __call__(self, *args, **kwargs) -> Any:
        return self.value(*args, **kwargs)

    def random(self, rng: Random) -> Any:
        raise NotImplementedError()

    def is_instance(self, value: Any) -> bool:
        raise NotImplementedError()

    def c_type(self) -> Union[CType, str]:
        raise NotImplementedError()

    def cpp_type(self) -> Union[CType, str]:
        return self.c_type()

    def c_source(self) -> Source:
        return None

    def cpp_source(self) -> Source:
        return self.c_source()

    def c_size(self, obj: str) -> str:
        raise NotImplementedError()

    def cpp_size(self, obj: str) -> str:
        return self.c_size(obj)

    def _c_size_extent(self, obj: str) -> str:
        raise NotImplementedError()

    def _cpp_size_extent(self, obj: str) -> str:
        raise NotImplementedError()

    def cpp_load(self, src: str) -> str:
        raise NotImplementedError()
    
    def cpp_store(self, src: str, dst: str) -> str:
        raise NotImplementedError()

    def cpp_object(self, value: Any) -> str:
        raise NotImplementedError()

    def c_test(self, obj: str, src: str) -> str:
        raise NotImplementedError()

    def cpp_test(self, dst: str, src: str) -> str:
        raise NotImplementedError()

    def _cpp_static_check(self) -> str:
        return f"static_assert(sizeof({self.c_type()}) == size_t({self.size() if self.sized else self.min_size()}));"

    def test_source(self) -> Source:
        rng = Random(0xdeadbeef)
        static_check = self._cpp_static_check()
        return Source(
            Location.TESTS,
            "\n".join([
                f"TEST({Name(CONTEXT.prefix, 'test').camel()}, {self.name().camel()}) {{",
                indent_text("\n".join([
                    *([
                        static_check,
                        f"",
                    ] if static_check is not None else []),
                    f"std::vector<{self.cpp_type()}> srcs = {{",
                    *[indent_text(self.cpp_object(self.random(rng)), "    ") + "," for _ in range(CONTEXT.test_attempts)],
                    f"}};",
                    f"",
                    f"for (size_t k = 0; k < {CONTEXT.test_attempts}; ++k) {{",
                    indent_text("\n".join([
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
                    ]), "    "),
                    f"}}",
                ]), "    "),
                f"}}",
            ]),
            deps=[ty.test_source() for ty in self.deps()],
        )

class SizedType(Type):
    def __init__(self, *args, **kwargs):
        super().__init__(sized=True, *args, **kwargs)

    def size(self) -> int:
        raise NotImplementedError()

    def min_size(self) -> int:
        return self.size()

    def c_size(self, obj: str) -> str:
        return str(self.size())

class TrivialType(SizedType):
    def __init__(self, *args, **kwargs):
        super().__init__(trivial=True, *args, **kwargs)

    def cpp_load(self, src: str) -> str:
        return f"{src}"
    
    def cpp_store(self, src: str, dst: str) -> str:
        return f"{dst} = {src}"

    def c_test(self, obj: str, src: str) -> str:
        return f"EXPECT_EQ({self.cpp_load(obj)}, {src});"

    def cpp_test(self, dst: str, src: str) -> str:
        return f"EXPECT_EQ({dst}, {src});"

    def test_source(self) -> Source:
        return None
