from __future__ import annotations
from dataclasses import dataclass

from ipp.base import Include, Type, Source
from ipp.serialize import LoadStatus, StoreStatus, LoadFn, StoreFn
from ipp.util import ceil_to_power_of_2

class Invariant(Type):
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

    def c_len(self, obj: str = None) -> str:
        raise NotImplementedError()

    def c_load(self, dst: str, src: str, max_len: str) -> str:
        return LoadFn(self).c_call(dst, src, max_len)

    def c_store(self, src: str, dst: str, max_len: str, len: str) -> str:
        return StoreFn(self).c_call(dst, src, max_len, len)

    def c_source(self) -> Source:
        return Source(
            [
                self._c_load_decl(),
                self._c_store_decl(),
            ],
            deps=[
                Source(["#include <string.h>"]),
                LoadStatus().c_source(),
                StoreStatus().c_source(),
            ],
        )
    
    def cpp_source(self) -> Source:
        return Source()

@dataclass
class Pointer(Type):
    type: Type
    const: bool = False

    def name(self):
        return f"{self.type.name()}{'Const' if self.const else ''}Ptr"

    def _ptr_type(self, type_str: str) -> str:
        return f"{'const ' if self.const else ''}{type_str} *"

    def c_type(self) -> str:
        return self._ptr_type(self.type.c_type())
    
    def cpp_type(self) -> str:
        return self._ptr_type(self.type.cpp_type())

    def c_source(self) -> Source:
        return self.type.c_source()

    def cpp_source(self) -> Source:
        return self.type.cpp_source()

@dataclass
class Int(Invariant):
    size: int
    signed: bool = False

    def name(self):
        return f"{'Ui' if not self.signed else 'I'}nt{self.size}"

    def c_type(self) -> str:
        if self.size % 8 != 0 or self.size > 64:
            raise RuntimeError(f"{self.size}-bit integer is not supported")
        # We assume that we use a LittleEndian CPU.
        # So the load/store functionality will work correctly.
        return f"{'u' if not self.signed else ''}int{ceil_to_power_of_2(self.size)}_t"

    def c_source(self) -> Source:
        return Source(deps=[
            Include("stdint.h"),
            super().c_source(),
        ])

    def cpp_source(self) -> Source:
        return Source(deps=[
            Include("cstdint"),
            super().cpp_source(),
        ])

    def c_len(self, obj: str = None) -> str:
        return f"{self.size // 8}"

@dataclass
class Float(Invariant):
    size: int

    def name(self):
        return f"Float{self.size}"

    def c_type(self) -> str:
        if self.size == 32:
            return "float"
        elif self.size == 64:
            return "double"
        else:
            raise RuntimeError(f"{self.size}-bit float is not supported")

    def c_len(self, obj: str = None) -> str:
        return f"{self.size // 8}"

@dataclass
class Char(Invariant):
    def name(self):
        return "Char"

    def c_type(self) -> str:
        return "char"

    def c_len(self, obj: str = None) -> str:
        return "1"

@dataclass
class Size(Type):
    signed: bool = False

    def name(self):
        return f"{'Us' if not self.signed else 'S'}ize"

    def c_type(self) -> str:
        return "size_t"

    def c_len(self, obj: str = None) -> str:
        return "sizeof(size_t)"
