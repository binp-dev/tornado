from __future__ import annotations
from typing import Any, List, Optional

from random import Random
from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

from ferrite.codegen.base import CONTEXT, Include, Location, Name, Type, Source
from ferrite.codegen.primitive import Char, Int
from ferrite.codegen.utils import indent
from ferrite.codegen.macros import ErrorKind, err, io_error, ok, stream_read, stream_write, try_unwrap


class _ItemBase(Type):

    def __init__(self, item: Type, sized: bool) -> None:
        assert item.sized
        self.item = item
        super().__init__(sized=sized)

    def deps(self) -> List[Type]:
        return [self.item]


class _ArrayBase(_ItemBase):

    def _load_array(self, data: bytes, size: int) -> List[Any] | NDArray[Any]:
        item_size = self.item.size()
        assert len(data) == item_size * size
        if not self.item.trivial:
            array = []
            for i in range(size):
                array.append(self.item.load(data[(i * item_size):((i + 1) * item_size)]))
            return array
        else:
            return np.frombuffer(data, self.item.np_dtype(), size)

    def _store_array(self, array: List[Any] | NDArray[Any]) -> bytes:
        if not self.item.trivial:
            assert isinstance(array, list)
            data = b''
            for item in array:
                data += self.item.store(item)
            return data
        else:
            assert isinstance(array, np.ndarray) and array.dtype == self.item.np_dtype()
            return array.tobytes()

    def _random_array(self, rng: Random, size: int) -> List[Any] | NDArray[Any]:
        array = [self.item.random(rng) for _ in range(size)]
        if not self.item.trivial:
            return array
        else:
            return np.array(array, dtype=self.item.np_dtype())

    def is_instance(self, value: List[Any] | NDArray[Any]) -> bool:
        if not self.item.trivial:
            return isinstance(value, list)
        else:
            return isinstance(value, np.ndarray) and value.dtype == self.item.np_dtype()

    def pyi_type(self) -> str:
        if not self.item.trivial:
            return f"List[{self.item.pyi_type()}]"
        else:
            return f"NDArray[{self.item.pyi_np_dtype()}]"

    def pyi_source(self) -> Optional[Source]:
        if not self.item.trivial:
            imports = [["from typing import List"]]
        else:
            imports = [["import numpy as np"], ["from numpy.typing import NDArray"]]
        return Source(Location.INCLUDES, imports)


class Array(_ArrayBase):

    def __init__(self, item: Type, len: int) -> None:
        super().__init__(item, sized=True)
        self.len = len

    def name(self) -> Name:
        return Name(f"array{self.len}", self.item.name())

    def size(self) -> int:
        return self.item.size() * self.len

    def load(self, data: bytes) -> List[Any] | NDArray[Any]:
        return self._load_array(data, self.len)

    def store(self, array: List[Any] | NDArray[Any]) -> bytes:
        assert len(array) == self.len
        return self._store_array(array)

    def random(self, rng: Random) -> List[Any] | NDArray[Any]:
        return self._random_array(rng, self.len)

    def is_instance(self, value: List[Any] | NDArray[Any]) -> bool:
        return len(value) == self.len and super().is_instance(value)

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
            f"    return {ok('std::move(dst)')};",
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
            f"    return {ok()};",
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


@dataclass
class _BasicVector(_ItemBase):

    def __init__(self, item: Type) -> None:
        super().__init__(item, sized=False)
        self._size_type = Int(16)

    def name(self) -> Name:
        return Name("vector", self.item.name())

    def min_size(self) -> int:
        return self._size_type.size()

    def deps(self) -> List[Type]:
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


class Vector(_BasicVector, _ArrayBase):

    def __init__(self, item: Type):
        super().__init__(item)

    def load(self, data: bytes) -> List[Any] | NDArray[Any]:
        count = self._size_type.load(data[:self._size_type.size()])
        data = data[self._size_type.size():]
        return self._load_array(data, count)

    def store(self, array: List[Any] | NDArray[Any]) -> bytes:
        return self._size_type.store(len(array)) + self._store_array(array)

    def random(self, rng: Random) -> List[Any] | NDArray[Any]:
        size = rng.randrange(0, 8)
        return self._random_array(rng, size)

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
            f"    return {ok('std::move(dst)')};",
            f"}}",
        ]
        store_src = [
            f"{self._cpp_store_func_decl('stream', 'src')} {{",
            f"    auto len_opt = core::safe_cast<{self._size_type.cpp_type()}>(src.size());",
            f"    if (!len_opt.has_value()) {{ return {err(io_error(ErrorKind.INVALID_DATA))}; }}",
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
            f"    return {ok()};",
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

    def cpp_object(self, value: List[Any]) -> str:
        return f"{self.cpp_type()}{{{', '.join([self.item.cpp_object(v) for v in value])}}}"


class String(_BasicVector):

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
            f"    return {ok('std::move(dst)')};",
            f"}}",
        ]
        store_src = [
            f"{store_decl} {{",
            f"    auto len_opt = core::safe_cast<{self._size_type.cpp_type()}>(src.size());",
            f"    if (!len_opt.has_value()) {{ return {err(io_error(ErrorKind.INVALID_DATA))}; }}",
            *indent(try_unwrap(self._size_type.cpp_store("stream", "len_opt.value()"))),
            *indent(try_unwrap(stream_write("stream", "src.data()", f"src.length()"))),
            f"    return {ok()};",
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
