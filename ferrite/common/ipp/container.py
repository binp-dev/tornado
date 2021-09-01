from __future__ import annotations
from typing import List

from ipp.base import Include, Name, Type, Source
from ipp.prim import Array, Char, Int
from ipp.struct import Struct, Field

class Vector(Type):
    def __init__(self, item: Type):
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
    
    def c_size(self, obj: str) -> str:
        return f"({self.min_size()} + obj.len)"

    def c_type(self) -> str:
        return self._c_struct.c_type()

    def cpp_type(self) -> str:
        return f"std::vector<{self.item.cpp_type()}>"

    def c_source(self) -> Source:
        return self._c_struct.c_source()

    def cpp_source(self) -> Source:
        return Source(deps=[
            Include("vector"),
            self.item.cpp_source(),
        ])

class String(Vector):
    def __init__(self):
        super().__init__(Char())

    def name(self) -> Name:
        return Name("string")

    def cpp_type(self) -> str:
        return "std::string"

    def cpp_source(self) -> Source:
        return Include("string")
