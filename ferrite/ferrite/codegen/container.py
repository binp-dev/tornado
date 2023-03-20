from __future__ import annotations
from typing import Any, Generic, Iterable, List, TypeVar, Union

from random import Random
from dataclasses import dataclass

from ferrite.codegen.base import CONTEXT, Include, Location, Name, Type, Source
from ferrite.codegen.primitive import Char, Int, Pointer, Reference
from ferrite.codegen.utils import indent_text

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
            "\n".join([
                f"typedef struct __attribute__((packed, aligned(1))) {{",
                f"    {self.item.c_type()} data[{self.len}];",
                f"}} {name};",
            ]),
            deps=[self.item.c_source()]
        )

    def _cpp_load_decl(self) -> str:
        return f"{self.cpp_type()} {Name(self.name(), 'load').snake()}({Pointer(self, const=True).c_type()} src)"

    def _cpp_store_decl(self) -> str:
        return f"void {Name(self.name(), 'store').snake()}({Reference(self, const=True).cpp_type()} src, {Pointer(self).c_type()} dst)"

    def _cpp_source_decl(self) -> Source:
        return Source(
            Location.DECLARATION,
            "\n".join([
                f"{self._cpp_load_decl()};",
                f"{self._cpp_store_decl()};",
            ]),
            deps=[
                Include("array"),
                self.item.cpp_source(),
            ]
        )

    def cpp_source(self) -> Source:
        if self.len is None:
            raise NotImplementedError()

        load_src = "\n".join([
            f"{self._cpp_load_decl()} {{",
            f"    {self.cpp_type()} dst;",
            f"    for (size_t i = 0; i < dst.size(); ++i) {{",
            f"        dst[i] = {self.item.cpp_load('src->data[i]')};",
            f"    }}",
            f"    return dst;",
            f"}}",
        ])
        store_src = "\n".join([
            f"{self._cpp_store_decl()} {{",
            f"    for (size_t i = 0; i < src.size(); ++i) {{",
            f"        {self.item.cpp_store('src[i]', 'dst->data[i]')};",
            f"    }}",
            f"}}",
        ])
        return Source(
            Location.DEFINITION, [
                load_src,
                store_src,
            ], deps=[
                self._cpp_source_decl(),
            ]
        )

    def cpp_load(self, src: str) -> str:
        return f"{Name(self.name(), 'load').snake()}(&{src})"

    def cpp_store(self, src: str, dst: str) -> str:
        return f"{Name(self.name(), 'store').snake()}({src}, &{dst});"

    def cpp_object(self, value: List[Any]) -> str:
        assert self.len == len(value)
        return f"{self.cpp_type()}{{{', '.join([self.item.cpp_object(v) for v in value])}}}"

    def c_test(self, obj: str, src: str) -> str:
        return "\n".join([
            f"ASSERT_EQ(size_t({self.len}), {src}.size());", f"for (size_t i = 0; i < {src}.size(); ++i) {{",
            indent_text(self.item.c_test(f"{obj}.data[i]", f"{src}[i]"), "    "), f"}}"
        ])

    def cpp_test(self, dst: str, src: str) -> str:
        return "\n".join([
            f"ASSERT_EQ({dst}.size(), {src}.size());", f"for (size_t i = 0; i < {src}.size(); ++i) {{",
            indent_text(self.item.cpp_test(f"{dst}[i]", f"{src}[i]"), "    "), f"}}"
        ])


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
            "\n".join([
                f"typedef struct __attribute__((packed, aligned(1))) {{",
                f"    {self._size_type.c_type()} len;",
                f"    {self.item.c_type()} data[];",
                f"}} {name};",
            ]),
            deps=[
                self.item.c_source(),
                self._size_type.c_source(),
            ]
        )

    def c_size(self, obj: str) -> str:
        return f"((size_t){self.min_size()} + ({obj}.len * {self.item.size()}))"

    def _c_size_extent(self, obj: str) -> str:
        item_size = self.item.size()
        return f"((size_t){obj}.len{f' * {item_size}' if item_size != 1 else ''})"

    def _cpp_size_extent(self, obj: str) -> str:
        item_size = self.item.size()
        return f"({obj}.size(){f' * {item_size}' if item_size != 1 else ''})"

    def cpp_type(self) -> str:
        return f"std::vector<{self.item.cpp_type()}>"

    def _cpp_load_decl(self) -> str:
        return f"{self.cpp_type()} {Name(self.name(), 'load').snake()}({Pointer(self, const=True).c_type()} src)"

    def _cpp_store_decl(self) -> str:
        return f"void {Name(self.name(), 'store').snake()}({Reference(self, const=True).cpp_type()} src, {Pointer(self).c_type()} dst)"

    def _cpp_source_decl(self) -> Source:
        return Source(
            Location.DECLARATION,
            "\n".join([
                f"{self._cpp_load_decl()};",
                f"{self._cpp_store_decl()};",
            ]),
            deps=[
                Include("vector"),
                self.item.cpp_source(),
            ]
        )

    def cpp_source(self) -> Source:
        load_src = "\n".join([
            f"{self._cpp_load_decl()} {{",
            f"    {self.cpp_type()} dst(static_cast<size_t>(src->len));",
            f"    for (size_t i = 0; i < dst.size(); ++i) {{",
            f"        dst[i] = {self.item.cpp_load('src->data[i]')};",
            f"    }}",
            f"    return dst;",
            f"}}",
        ])
        store_src = "\n".join([
            f"{self._cpp_store_decl()} {{",
            f"    // FIXME: Check for `dst->len` overflow.",
            f"    dst->len = static_cast<{self._size_type.c_type()}>(src.size());",
            f"    for (size_t i = 0; i < src.size(); ++i) {{",
            f"        {self.item.cpp_store('src[i]', 'dst->data[i]')};",
            f"    }}",
            f"}}",
        ])
        return Source(
            Location.DEFINITION, [
                load_src,
                store_src,
            ], deps=[
                self._cpp_source_decl(),
            ]
        )

    def cpp_size(self, obj: str) -> str:
        return f"({self.min_size()} + {self._cpp_size_extent(obj)})"

    def cpp_load(self, src: str) -> str:
        return f"{Name(self.name(), 'load').snake()}(&{src})"

    def cpp_store(self, src: str, dst: str) -> str:
        return f"{Name(self.name(), 'store').snake()}({src}, &{dst});"

    def c_test(self, obj: str, src: str) -> str:
        return "\n".join([
            f"ASSERT_EQ({obj}.len, {src}.size());", f"for (size_t i = 0; i < {src}.size(); ++i) {{",
            indent_text(self.item.c_test(f"{obj}.data[i]", f"{src}[i]"), "    "), f"}}"
        ])

    def cpp_test(self, dst: str, src: str) -> str:
        return "\n".join([
            f"ASSERT_EQ({dst}.size(), {src}.size());", f"for (size_t i = 0; i < {src}.size(); ++i) {{",
            indent_text(self.item.cpp_test(f"{dst}[i]", f"{src}[i]"), "    "), f"}}"
        ])


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

    def cpp_object(self, value: List[T]) -> str:
        return f"{self.cpp_type()}{{{', '.join([self.item.cpp_object(v) for v in value])}}}"


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
        load_decl = f"{self.cpp_type()} {Name(self.name(), 'load').snake()}({Pointer(self, const=True).c_type()} src)"
        store_decl = f"void {Name(self.name(), 'store').snake()}({Reference(self, const=True).cpp_type()} src, {Pointer(self).c_type()} dst)"
        load_src = "\n".join([
            f"{load_decl} {{",
            f"    return {self.cpp_type()}(src->data, static_cast<size_t>(src->len));",
            f"}}",
        ])
        store_src = "\n".join([
            f"{store_decl} {{",
            f"    // FIXME: Check for `dst->len` overflow.",
            f"    dst->len = static_cast<{self._size_type.c_type()}>(src.size());",
            f"    memcpy((void *)dst->data, (const void *)src.c_str(), src.length());",
            f"}}",
        ])
        return Source(
            Location.DEFINITION, [
                load_src,
                store_src,
            ],
            deps=[Source(Location.DECLARATION, "\n".join([f"{load_decl};"
                                                          f"{store_decl};"]), deps=[Include("string")])]
        )

    def cpp_object(self, value: str) -> str:
        return f"{self.cpp_type()}(\"{value}\")"

    def c_test(self, obj: str, src: str) -> str:
        return "\n".join([
            f"ASSERT_EQ({obj}.len, {src}.size());",
            f"EXPECT_EQ(strncmp({obj}.data, {src}.c_str(), {src}.size()), 0);",
        ])

    def cpp_test(self, dst: str, src: str) -> str:
        return "\n".join([
            f"ASSERT_EQ({dst}, {src});",
        ])
