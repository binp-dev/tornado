from __future__ import annotations
from typing import List

from ipp.base import Include, Name, SizedType, Type, Source
from ipp.prim import Array, Char, Int, Pointer, Reference
from ipp.struct import Struct, Field

class Vector(Type):
    def __init__(self, item: SizedType):
        assert item.sized
        super().__init__()
        self.item = item
        self._c_struct = Struct(
            self.name(),
            [
                Field("len", Int(16)),
                Field("data", Array(self.item, 0)),
            ],
        )

    def name(self) -> Name:
        return Name("vector", self.item.name())

    def min_size(self) -> int:
        return self._c_struct.fields[0].type.size()

    def _c_size_extent(self, obj: str) -> str:
        return f"((size_t){obj}.len * {self.item.size()})"

    def c_size(self, obj: str) -> str:
        return f"({self.min_size()} + {self._c_size_extent(obj)})"

    def c_type(self) -> str:
        return self._c_struct.c_type()

    def cpp_type(self) -> str:
        return f"std::vector<{self.item.cpp_type()}>"

    def c_source(self) -> Source:
        return self._c_struct.c_source()

    def cpp_source(self) -> Source:
        load_src = "\n".join([
            f"{self.cpp_type()} {Name(self.name(), 'load').snake()}({Pointer(self, const=True).c_type()} src) {{",
            f"    {self.cpp_type()} dst(static_cast<size_t>(src->len));",
            *([
                f"    memcpy((void *)dst.data(), (const void *)src->data, {self.c_size('src')});",
            ] if self.item.trivial else [
                f"    for (size_t i = 0; i < dst.size(); ++i) {{",
                f"        {self.item.cpp_load('dst[i]', 'src->data[i]')};",
                f"    }}",
            ]),
            f"    return dst;",
            f"}}",
        ])
        store_src = "\n".join([
            f"void {Name(self.name(), 'store').snake()}({Reference(self, const=True).cpp_type()} src, {Pointer(self).c_type()} dst) {{",
            f"    // FIXME: Check for `dst->len` overflow.",
            f"    dst->len = static_cast<{self._c_struct.fields[0].type.c_type()}>(src.size());",
            *([
                f"    memcpy((void *)dst->data, (const void *)src.data(), src.size() * {self.item.size()});",
            ] if self.item.trivial else [
                f"    for (size_t i = 0; i < src.size(); ++i) {{",
                f"        {self.item.cpp_store('src[i]', 'dst->data[i]')};",
                f"    }}",
            ]),
            f"}}",
        ])
        return Source([
            load_src,
            store_src,
        ], deps=[
            Include("vector"),
            self.item.cpp_source(),
        ])

    def cpp_size(self, obj: str) -> str:
        return f"({obj}.size() * {self.item.size()})"

    def cpp_load(self, dst: str, src: str) -> str:
        return f"{dst} = {Name(self.name(), 'load').snake()}(&{src})"
    
    def cpp_store(self, src: str, dst: str) -> str:
        return f"{Name(self.name(), 'store').snake()}({src}, &{dst})"

class String(Vector):
    def __init__(self):
        super().__init__(Char())

    def name(self) -> Name:
        return Name("string")

    def cpp_type(self) -> str:
        return "std::string"

    def cpp_source(self) -> Source:
        return Include("string")
