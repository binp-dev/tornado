from __future__ import annotations
from typing import Any, Generic, List, Optional, TypeVar

from random import Random
from dataclasses import dataclass

from ferrite.codegen.base import CONTEXT, Include, Location, Name, Type, Source
from ferrite.codegen.primitive import Char, Int, Pointer, Reference
from ferrite.codegen.utils import indent
from ferrite.codegen.macros import ErrorKind, io_error, monostate, stream_read, stream_write, try_unwrap

T = TypeVar("T")


@dataclass
class Array(Type[List[T]]):
    item: Type[T]
    len: int

    def __post_init__(self) -> None:
        assert self.item.sized
        super().__init__(
            sized=True,
            trivial=self.item.trivial,
        )

    def name(self) -> Name:
        return Name(f"array{self.len}", self.item.name())

    def size(self) -> int:
        return self.item.size() * self.len

    def load(self, data: bytes) -> List[T]:
        item_size = self.item.size()
        assert len(data) == item_size * self.len
        array = []
        for i in range(self.len):
            array.append(self.item.load(data[(i * item_size):((i + 1) * item_size)]))
        return array

    def store(self, array: List[T]) -> bytes:
        data = b''
        for item in array:
            data += self.item.store(item)
        return data

    def random(self, rng: Random) -> List[T]:
        return [self.item.random(rng) for _ in range(self.len)]

    def is_instance(self, value: List[T]) -> bool:
        return isinstance(value, list) and len(value) == self.len

    def deps(self) -> List[Type[Any]]:
        return [self.item]

    def c_size(self, obj: str) -> str:
        return str(self.size())

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name()).camel()

    def cpp_type(self) -> str:
        return f"std::array<{self.item.cpp_type()}, {self.len}>"

    def c_source(self) -> Source:
        name = self.c_type()
        return Source(
            Location.DECLARATION,
            [[
                f"typedef struct __attribute__((packed, aligned(1))) {{",
                f"    {self.item.c_type()} data[{self.len}];",
                f"}} {name};",
            ]],
            deps=[self.item.c_source()],
        )

    def _cpp_source_decl(self) -> Source:
        return Source(
            Location.DECLARATION,
            [
                [f"{self._cpp_load_func_decl('stream')};"],
                [f"{self._cpp_store_func_decl('stream', 'src')};"],
            ],
            deps=[
                Include("array"),
                self.item.cpp_source(),
            ],
        )

    def cpp_source(self) -> Source:
        if self.len is None:
            raise NotImplementedError()

        load_src = [
            f"{self._cpp_load_func_decl('stream')} {{",
            f"    {self.cpp_type()} dst;",
            *indent([
                [
                    f"for (size_t i = 0; i < dst.size(); ++i) {{",
                    *indent(try_unwrap(self.item.cpp_load("stream"), lambda x: f"dst[i] = {x};")),
                    f"}}",
                ],
                try_unwrap(stream_read(
                    "stream",
                    "dst.data()",
                    self.item.size() * self.len,
                )),
            ][self.item.trivial]),
            f"    return Ok(std::move(dst));",
            f"}}",
        ]
        store_src = [
            f"{self._cpp_store_func_decl('stream', 'src')} {{",
            *indent([
                [
                    f"for (size_t i = 0; i < src.size(); ++i) {{",
                    *indent(try_unwrap(self.item.cpp_store("stream", "src[i]"))),
                    f"}}",
                ],
                try_unwrap(stream_write(
                    "stream",
                    "src.data()",
                    self.item.size() * self.len,
                )),
            ][self.item.trivial]),
            f"    return Ok({monostate()});",
            f"}}",
        ]
        return Source(
            Location.DEFINITION, [
                load_src,
                store_src,
            ], deps=[
                self._cpp_source_decl(),
            ]
        )

    def cpp_load(self, stream: str) -> str:
        return self._cpp_load_func(stream)

    def cpp_store(self, stream: str, dst: str) -> str:
        return self._cpp_store_func(stream, dst)

    def cpp_object(self, value: List[Any]) -> str:
        assert self.len == len(value)
        return f"{self.cpp_type()}{{{', '.join([self.item.cpp_object(v) for v in value])}}}"

    def c_test(self, obj: str, src: str) -> List[str]:
        return [
            f"ASSERT_EQ(size_t({self.len}), {src}.size());",
            f"for (size_t i = 0; i < {src}.size(); ++i) {{",
            *["    " + s for s in self.item.c_test(f"{obj}.data[i]", f"{src}[i]")],
            f"}}",
        ]

    def cpp_test(self, dst: str, src: str) -> List[str]:
        return [
            f"ASSERT_EQ({dst}.size(), {src}.size());",
            f"for (size_t i = 0; i < {src}.size(); ++i) {{",
            *indent(self.item.cpp_test(f"{dst}[i]", f"{src}[i]")),
            f"}}",
        ]

    def pyi_type(self) -> str:
        return f"List[{self.item.pyi_type()}]"

    def pyi_source(self) -> Optional[Source]:
        return Source(Location.INCLUDES, [["from typing import List"]])


V = TypeVar('V')


@dataclass
class _BasicVector(Generic[V, T], Type[V]):
    item: Type[T]

    def __post_init__(self) -> None:
        assert self.item.sized
        super().__init__(sized=False)
        self._size_type = Int(16)

    def name(self) -> Name:
        return Name("vector", self.item.name())

    def min_size(self) -> int:
        return self._size_type.size()

    def deps(self) -> List[Type[Any]]:
        return [self.item, self._size_type]

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name()).camel()

    def c_source(self) -> Source:
        name = self.c_type()
        return Source(
            Location.DECLARATION,
            [[
                f"typedef struct __attribute__((packed, aligned(1))) {{",
                f"    {self._size_type.c_type()} len;",
                f"    {self.item.c_type()} data[];",
                f"}} {name};",
            ]],
            deps=[
                self.item.c_source(),
                self._size_type.c_source(),
            ],
        )

    def c_size(self, obj: str) -> str:
        return f"((size_t){self.min_size()} + ({obj}.len * {self.item.size()}))"

    def _c_size_extent(self, obj: str) -> str:
        item_size = self.item.size()
        return f"((size_t){obj}.len{f' * {item_size}' if item_size != 1 else ''})"

    def _cpp_size_extent(self, obj: str) -> str:
        item_size = self.item.size()
        return f"({obj}.size(){f' * {item_size}' if item_size != 1 else ''})"

    def cpp_size(self, obj: str) -> str:
        return f"({self.min_size()} + {self._cpp_size_extent(obj)})"

    def cpp_load(self, stream: str) -> str:
        return self._cpp_load_func(stream)

    def cpp_store(self, stream: str, dst: str) -> str:
        return self._cpp_store_func(stream, dst)

    def c_test(self, obj: str, src: str) -> List[str]:
        return [
            f"ASSERT_EQ({obj}.len, {src}.size());",
            f"for (size_t i = 0; i < {src}.size(); ++i) {{",
            *indent(self.item.c_test(f"{obj}.data[i]", f"{src}[i]")),
            f"}}",
        ]

    def cpp_test(self, dst: str, src: str) -> List[str]:
        return [
            f"ASSERT_EQ({dst}.size(), {src}.size());",
            f"for (size_t i = 0; i < {src}.size(); ++i) {{",
            *indent(self.item.cpp_test(f"{dst}[i]", f"{src}[i]")),
            f"}}",
        ]


class Vector(Generic[T], _BasicVector[List[T], T]):

    def __init__(self, item: Type[T]):
        super().__init__(item)

    def load(self, data: bytes) -> List[T]:
        count = self._size_type.load(data[:self._size_type.size()])
        data = data[self._size_type.size():]
        item_size = self.item.size()
        assert len(data) == item_size * count
        array = []
        for i in range(count):
            array.append(self.item.load(data[(i * item_size):((i + 1) * item_size)]))
        return array

    def store(self, array: List[T]) -> bytes:
        data = b''
        data += self._size_type.store(len(array))
        for item in array:
            data += self.item.store(item)
        return data

    def random(self, rng: Random) -> List[T]:
        size = rng.randrange(0, 8)
        return [self.item.random(rng) for _ in range(size)]

    def is_instance(self, value: List[T]) -> bool:
        return isinstance(value, list)

    def cpp_type(self) -> str:
        return f"std::vector<{self.item.cpp_type()}>"

    def _cpp_source_decl(self) -> Source:
        return Source(
            Location.DECLARATION,
            [
                [f"{self._cpp_load_func_decl('stream')};"],
                [f"{self._cpp_store_func_decl('stream', 'src')};"],
            ],
            deps=[
                Include("vector"),
                Include("core/convert.hpp"),
                self.item.cpp_source(),
                self._size_type.cpp_source(),
            ],
        )

    def cpp_source(self) -> Source:
        load_src = [
            f"{self._cpp_load_func_decl('stream')} {{",
            *indent(try_unwrap(self._size_type.cpp_load("stream"), lambda l: f"{self._size_type.cpp_type()} len = {l};")),
            f"    auto dst = {self.cpp_type()}(static_cast<size_t>(len));",
            *indent([
                [
                    f"for (size_t i = 0; i < dst.size(); ++i) {{",
                    *indent(try_unwrap(self.item.cpp_load("stream"), lambda x: f"dst[i] = {x};")),
                    f"}}",
                ],
                try_unwrap(stream_read(
                    "stream",
                    "dst.data()",
                    f"({self.item.size()} * dst.size())",
                )),
            ][self.item.trivial]),
            f"    return Ok(std::move(dst));",
            f"}}",
        ]
        store_src = [
            f"{self._cpp_store_func_decl('stream', 'src')} {{",
            f"    auto len_opt = safe_cast<{self._size_type.cpp_type()}>(src.size());",
            f"    if (!len_opt.has_value()) {{ return Err({io_error(ErrorKind.INVALID_DATA)}); }}",
            *indent(try_unwrap(self._size_type.cpp_store("stream", "len_opt.value()"))),
            *indent([
                [
                    f"for (size_t i = 0; i < src.size(); ++i) {{",
                    *indent(try_unwrap(self.item.cpp_store("stream", "src[i]"))),
                    f"}}",
                ],
                try_unwrap(stream_write(
                    "stream",
                    "src.data()",
                    f"({self.item.size()} * src.size())",
                )),
            ][self.item.trivial]),
            f"    return Ok({monostate()});",
            f"}}",
        ]
        return Source(
            Location.DEFINITION, [
                load_src,
                store_src,
            ], deps=[
                self._cpp_source_decl(),
            ]
        )

    def cpp_object(self, value: List[T]) -> str:
        return f"{self.cpp_type()}{{{', '.join([self.item.cpp_object(v) for v in value])}}}"

    def pyi_type(self) -> str:
        return f"List[{self.item.pyi_type()}]"

    def pyi_source(self) -> Optional[Source]:
        return Source(Location.INCLUDES, [["from typing import List"]])


class String(_BasicVector[str, str]):

    def __init__(self) -> None:
        super().__init__(Char())

    def name(self) -> Name:
        return Name("string")

    def load(self, data: bytes) -> str:
        count = self._size_type.load(data[:self._size_type.size()])
        data = data[self._size_type.size():]
        assert len(data) == count
        return data.decode("ascii")

    def store(self, value: str) -> bytes:
        data = b''
        data += self._size_type.store(len(value))
        data += value.encode("ascii")
        return data

    def random(self, rng: Random) -> str:
        size = rng.randrange(0, 64)
        return "".join([Char().random(rng) for _ in range(size)])

    def is_instance(self, value: Any) -> bool:
        return isinstance(value, str)

    def cpp_type(self) -> str:
        return "std::string"

    def cpp_source(self) -> Source:
        load_decl = self._cpp_load_func_decl("stream")
        store_decl = self._cpp_store_func_decl("stream", "src")
        load_src = [
            f"{load_decl} {{",
            *indent(try_unwrap(self._size_type.cpp_load("stream"), lambda l: f"{self._size_type.cpp_type()} len = {l};")),
            f"    auto dst = {self.cpp_type()}(static_cast<size_t>(len), '\\0');",
            *indent(try_unwrap(stream_read("stream", "dst.data()", f"dst.length()"))),
            f"    return Ok(std::move(dst));",
            f"}}",
        ]
        store_src = [
            f"{store_decl} {{",
            f"    auto len_opt = safe_cast<{self._size_type.cpp_type()}>(src.size());",
            f"    if (!len_opt.has_value()) {{ return Err({io_error(ErrorKind.INVALID_DATA)}); }}",
            *indent(try_unwrap(self._size_type.cpp_store("stream", "len_opt.value()"))),
            *indent(try_unwrap(stream_write("stream", "src.data()", f"src.length()"))),
            f"    return Ok({monostate()});",
            f"}}",
        ]
        return Source(
            Location.DEFINITION,
            [
                load_src,
                store_src,
            ],
            deps=[
                Source(
                    Location.DECLARATION,
                    [
                        [f"{load_decl};"],
                        [f"{store_decl};"],
                    ],
                    deps=[
                        Include("string"),
                        self._size_type.cpp_source(),
                    ],
                )
            ],
        )

    def cpp_object(self, value: str) -> str:
        return f"{self.cpp_type()}(\"{value}\")"

    def c_test(self, obj: str, src: str) -> List[str]:
        return [
            f"ASSERT_EQ({obj}.len, {src}.size());",
            f"EXPECT_EQ(strncmp({obj}.data, {src}.c_str(), {src}.size()), 0);",
        ]

    def cpp_test(self, dst: str, src: str) -> List[str]:
        return [
            f"ASSERT_EQ({dst}, {src});",
        ]

    def pyi_type(self) -> str:
        return f"str"
