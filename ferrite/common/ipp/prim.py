from __future__ import annotations
from dataclasses import dataclass

from ipp.base import Type, Prelude
from ipp.util import is_power_of_2

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

    def c_prelude(self) -> Prelude:
        return self.type.c_prelude()

    def cpp_prelude(self) -> Prelude:
        return self.type.cpp_prelude()

@dataclass
class Int(Type):
    size: int
    signed: bool = False

    def name(self):
        return f"{'Ui' if not self.signed else 'I'}nt{self.size}"

    def c_type(self) -> str:
        if not is_power_of_2(self.size) or self.size > 64:
            raise RuntimeError(f"{self.size}-bit integer is not supported")
        return f"{'u' if not self.signed else ''}int{self.size}_t"

    def c_prelude(self) -> Prelude:
        return Prelude("#include <stdint.h>")

    def cpp_prelude(self) -> Prelude:
        return Prelude("#include <cstdint>")

@dataclass
class Float(Type):
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

class Char(Type):
    def __init__(self):
        super().__init__()

    def name(self):
        return "Char"

    def c_type(self) -> str:
        return "char"
