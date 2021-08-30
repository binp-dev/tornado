from __future__ import annotations
from typing import List

from ipp.base import Type, Source
from ipp.prim import Char, Int, Pointer
from ipp.struct import Struct, Field
from ipp.serialize import LoadStatus, StoreStatus, LoadFn, StoreFn

class Vector(Type):
    def __init__(self, item: Type):
        super().__init__()
        self.item = item
        self._c_struct = Struct(
            self.name(),
            [
                Field("len", Int(16)),
                Field("data", Pointer(Int(8), const=True)),
            ],
        )

    def name(self) -> str:
        return f"Vector{self.item.name()}"

    def c_type(self) -> str:
        return self._c_struct.c_type()

    def cpp_type(self) -> str:
        return f"std::vector<{self.item.cpp_type()}>"
    
    def _c_load_decl(self) -> str:
        return "\n".join([
            f"{LoadFn(self).c_decl()} {{",
            f"    *len = {self.c_len()};",
            f"    {LoadStatus().c_type()} code = IPP_LOAD_OK;",
            f"    if (max_len < *len) {{",
            f"        *len = max_len;",
            f"        code = IPP_LOAD_OUT_OF_BOUNDS;",
            f"    }}",
            f"    memcpy((void *)dst, (const void *)src, *len);",
            f"    return code;",
            f"}}",
        ])

    def _c_store_decl(self) -> str:
        return "\n".join([
            f"{StoreFn(self).c_decl()} {{",
            f"    *len = {self.c_len()};",
            f"    {StoreStatus().c_type()} code = IPP_STORE_OK;",
            f"    if (max_len < *len) {{",
            f"        *len = max_len;",
            f"        code = IPP_STORE_OUT_OF_BOUNDS;",
            f"    }}",
            f"    memcpy((void *)dst, (const void *)src, *len);",
            f"    return code;",
            f"}}",
        ])

    def c_source(self) -> Source:
        return self._c_struct.c_source()

    def cpp_source(self) -> Source:
        return Source(["#include <vector>"], [self.item.cpp_source()])

class String(Type):
    def __init__(self):
        super().__init__()
        self._c_struct = Struct(
            self.name(),
            [
                Field("len", Int(16)),
                Field("data", Pointer(Char(), const=True)),
            ],
        )

    def name(self) -> str:
        return "String"

    def c_type(self) -> str:
        return Pointer(Char()).c_type()

    def cpp_type(self) -> str:
        return "std::string"

    def cpp_source(self) -> Source:
        return Source(["#include <string>"])
