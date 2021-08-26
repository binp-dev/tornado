from __future__ import annotations
from dataclasses import dataclass

from ipp.base import Type, Prelude

@dataclass
class Pointer(Type):
    type: Type
    const: bool = False

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

    def c_type(self) -> str:
        return f"{'u' if not self.signed else ''}int{self.size}_t"

    def c_prelude(self) -> Prelude:
        return Prelude("#include <stdint.h>")

    def cpp_prelude(self) -> Prelude:
        return Prelude("#include <cstdint>")
