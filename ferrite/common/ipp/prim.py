from __future__ import annotations
from typing import List
from dataclasses import dataclass

from ipp.base import CType, Name, SizedType, Type, Source
from ipp.util import ceil_to_power_of_2, is_power_of_2

@dataclass
class Pointer(SizedType):
    type: Type
    const: bool = False

    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name(self.type.name(), "const" if self.const else "", + "ptr")

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
class Int(SizedType):
    bits: int
    signed: bool = False

    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name(("u" if not self.signed else "") + "int", str(self.bits))

    def size(self) -> int:
        return (self.bits - 1) // 8 + 1

    @staticmethod
    def _int_type(bits: int, signed: bool = False) -> str:
        return f"{'u' if not signed else ''}int{bits}_t"

    def c_type(self) -> str:
        return self._int_type(self.bits, self.signed)

    def c_source(self) -> Source:
        if self.bits % 8 != 0 or self.bits > 64:
            raise RuntimeError(f"{self.bits}-bit integer is not supported")
        bytes = self.bits // 8
        if is_power_of_2(bytes):
            return None
        else:
            if self.signed:
                raise RuntimeError(f"Signed integers are only supported to have power-of-2 size")
            name = self._int_type(self.bits)
            ceil_name = self._int_type(ceil_to_power_of_2(self.bits))
            return Source("\n".join([
                f"typedef struct {{",
                f"    uint8_t bytes[{bytes}];",
                f"}} {name};",
                f"",
                f"{ceil_name} to_uint{self.bits}({name} x) {{",
                f"    {ceil_name} y = 0;",
                f"    memcpy((void *)&y, (const void *)&x, {self.size()});",
                f"    return y;",
                f"}}",
                f"",
                f"{name} from_uint{self.bits}({ceil_name} y) {{",
                f"    {name} x;",
                f"    memcpy((void *)&x, (const void *)&y, {self.size()});",
                f"    return x;",
                f"}}",
            ]))

    def cpp_type(self) -> str:
        return self._int_type(ceil_to_power_of_2(self.bits))

    def cpp_source(self) -> Source:
        return None

@dataclass
class Float(SizedType):
    bits: int

    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name("float", str(self.bits))

    def size(self) -> int:
        return (self.bits - 1) // 8 + 1

    def c_type(self) -> str:
        if self.bits == 32:
            return "float"
        elif self.bits == 64:
            return "double"
        else:
            raise RuntimeError(f"{self.bits}-bit float is not supported")

@dataclass
class Char(SizedType):
    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name("char")

    def size(self) -> int:
        return 1

    def c_type(self) -> str:
        return "char"

@dataclass
class Size(SizedType):
    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name("usize")

    def c_type(self) -> str:
        return "size_t"

@dataclass
class Array(SizedType):
    type: SizedType
    len: int

    def __post_init__(self):
        super().__init__()

    def size(self) -> int:
        return self.type.size() * self.len

    def c_type(self) -> CType:
        return CType(str(self.type.c_type()), f"[{self.len}]")

    def c_source(self) -> Source:
        return self.type.c_source()

    def cpp_source(self) -> Source:
        return self.type.cpp_source()
