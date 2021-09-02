from __future__ import annotations
from typing import List

from ipp.base import CONTEXT, Include, Location, Name, SizedType, Type, Source
from ipp.prim import Array, Char, Int, Pointer, Reference
from ipp.struct import Struct, Field

class Vector(Struct):
    def __init__(self, item: SizedType):
        assert item.sized
        self.item = item
        super().__init__(
            self.name(),
            [
                Field("len", Int(16)),
                Field("data", Array(self.item, None)),
            ],
        )

    def name(self) -> Name:
        return Name("vector", self.item.name())

    def _c_size_extent(self, obj: str) -> str:
        item_size = self.item.size()
        return f"((size_t){obj}.len{f' * {item_size}' if item_size != 1 else ''})"

    def _cpp_size_extent(self, obj: str) -> str:
        item_size = self.item.size()
        return f"({obj}.size(){f' * {item_size}' if item_size != 1 else ''})"

    def cpp_type(self) -> str:
        return f"std::vector<{self.item.cpp_type()}>"

    def _cpp_load_decl(self):
        return f"{self.cpp_type()} {Name(self.name(), 'load').snake()}({Pointer(self, const=True).c_type()} src)"

    def _cpp_store_decl(self):
        return f"void {Name(self.name(), 'store').snake()}({Reference(self, const=True).cpp_type()} src, {Pointer(self).c_type()} dst)"

    def _cpp_source_decl(self) -> Source:
        return Source(Location.DECLARATION, "\n".join([
            f"{self._cpp_load_decl()};",
            f"{self._cpp_store_decl()};",
        ]), deps=[
            Include("vector"),
            self.item.cpp_source(),
        ])

    def cpp_source(self) -> Source:
        load_src = "\n".join([
            f"{self._cpp_load_decl()} {{",
            f"    {self.cpp_type()} dst(static_cast<size_t>(src->len));",
            *([
                f"    memcpy((void *)dst.data(), (const void *)src->data, {self.c_size('src')});",
            ] if self.item.trivial else [
                f"    for (size_t i = 0; i < dst.size(); ++i) {{",
                f"        dst[i] = {self.item.cpp_load('src->data[i]')};",
                f"    }}",
            ]),
            f"    return dst;",
            f"}}",
        ])
        store_src = "\n".join([
            f"{self._cpp_store_decl()} {{",
            f"    // FIXME: Check for `dst->len` overflow.",
            f"    dst->len = static_cast<{self.fields[0].type.c_type()}>(src.size());",
            *([
                f"    memcpy((void *)dst->data, (const void *)src.data(), src.size() * {self.item.size()});",
            ] if self.item.trivial else [
                f"    for (size_t i = 0; i < src.size(); ++i) {{",
                f"        {self.item.cpp_store('src[i]', 'dst->data[i]')};",
                f"    }}",
            ]),
            f"}}",
        ])
        return Source(Location.DEFINITION, [
            load_src,
            store_src,
        ], deps=[
            self._cpp_source_decl(),
        ])

    def cpp_size(self, obj: str) -> str:
        return f"({obj}.size() * {self.item.size()})"

    def cpp_load(self, src: str) -> str:
        return f"{Name(self.name(), 'load').snake()}(&{src})"
    
    def cpp_store(self, src: str, dst: str) -> str:
        return f"{Name(self.name(), 'store').snake()}({src}, &{dst});"

class String(Vector):
    def __init__(self):
        super().__init__(Char())

    def name(self) -> Name:
        return Name("string")

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
            f"    dst->len = static_cast<{self.fields[0].type.c_type()}>(src.size());",
            f"    memcpy((void *)dst->data, (const void *)src.c_str(), src.length());",
            f"}}",
        ])
        return Source(Location.DEFINITION, [
            load_src,
            store_src,
        ], deps=[Source(Location.DECLARATION, "\n".join([
            f"{load_decl};"
            f"{store_decl};"
        ]), deps=[Include("string")])])
