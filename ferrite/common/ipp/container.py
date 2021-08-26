from __future__ import annotations
from typing import List
from dataclasses import dataclass

from ipp.base import Type, Prelude
from ipp.prim import Int, Pointer
from ipp.struct import Struct, Field
from ipp.util import to_ident

class Vector(Type):
    def __init__(self, item: Type):
        super().__init__()
        self.item = item
        self._c_struct = Struct(
            "Vector_" + to_ident(self.item.c_type()),
            [
                Field("len", Int(16)),
                Field("data", Pointer(item)),
            ],
        )

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

    def c_type(self) -> str:
        return "char *"

    def cpp_type(self) -> str:
        return "std::string"

    def cpp_prelude(self) -> Prelude:
        return Prelude("#include <string>")
