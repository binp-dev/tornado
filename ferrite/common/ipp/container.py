from __future__ import annotations
from typing import List

from ipp.base import Type, Prelude
from ipp.prim import Char, Int, Pointer
from ipp.struct import Struct, Field

class Vector(Type):
    def __init__(self, item: Type):
        super().__init__()
        self.item = item
        self._c_struct = Struct(
            self.name(),
            [
                Field("len", Int(16)),
                Field("data", Pointer(item)),
            ],
        )

    def name(self) -> str:
        return f"Vector{self.item.name()}"

    def c_type(self) -> str:
        return self._c_struct.c_type()

    def cpp_type(self) -> str:
        return f"std::vector<{self.item.cpp_type()}>"

    def c_prelude(self) -> Prelude:
        return self._c_struct.c_prelude()

    def cpp_prelude(self) -> Prelude:
        return Prelude("#include <vector>", [self.item.cpp_prelude()])

class String(Type):
    def __init__(self):
        super().__init__()

    def name(self) -> str:
        return "String"

    def c_type(self) -> str:
        return Pointer(Char()).c_type()

    def cpp_type(self) -> str:
        return "std::string"

    def cpp_prelude(self) -> Prelude:
        return Prelude("#include <string>")
