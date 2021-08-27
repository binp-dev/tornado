from __future__ import annotations
from typing import List
from dataclasses import dataclass, fields

from ipp.base import Type, Source
from ipp.prim import Int

@dataclass
class Field:
    name: str
    type: Type

class Struct(Type):
    def __init__(self, name: str, fields: List[Field] = []):
        self._name = name
        self.fields = fields

    def name(self):
        return self._name

    def _c_definition(self) -> str:
        fields = [f"    {field.type.c_type()} {field.name};" for field in self.fields]
        if len(fields) == 0:
            fields = ["    char __placeholder;"]
        return "\n".join([
            f"typedef struct {self.name()} {{",
            *fields,
            f"}} {self.name()};",
        ])

    def _cpp_definition(self) -> str:
        return "\n".join([
            f"class {self.name()} final {{",
            f"public:",
            *[f"    {field.type.cpp_type()} {field.name};" for field in self.fields],
            f"}};",
        ])

    def c_type(self) -> str:
        return self.name()
    
    def cpp_type(self) -> str:
        return self.name()

    def c_source(self) -> Source:
        return Source(
            [self._c_definition()],
            [field.type.c_source() for field in self.fields],
        )
    
    def cpp_source(self) -> Source:
        return Source(
            [self._cpp_definition()],
            [field.type.cpp_source() for field in self.fields],
        )

class Variant(Type):
    def __init__(self, name: str, options: List[Field]):
        self._name = name
        self.options = options
        self._id_type = Int(8)

    def name(self):
        return self._name

    def _c_definition(self) -> str:
        return "\n".join([
            f"typedef struct {self.name()} {{",
            f"    {self._id_type.c_type()} type;",
            f"    union {{",
            *[f"        {option.type.c_type()} {option.name};" for option in self.options],
            f"    }};",
            f"}} {self.name()};",
        ])

    def _cpp_definition(self) -> str:
        return "\n".join([
            f"class {self.name()} final {{",
            f"public:",
            f"    std::variant<",
            *[
                f"        {option.type.cpp_type()}{',' if i < len(self.options) else ''}"
                for i, option in enumerate(self.options)
            ],
            f"    > variant;",
            f"}};",
        ])

    def c_type(self) -> str:
        return self.name()

    def cpp_type(self) -> str:
        return self.name()

    def c_source(self) -> Source:
        return Source(
            [self._c_definition()],
            [
                self._id_type.c_source(),
                *[option.type.c_source() for option in self.options],
            ],
        )

    def cpp_source(self) -> Source:
        return Source(
            [self._cpp_definition()],
            [
                Source(["#include <variant>"]),
                *[option.type.cpp_source() for option in self.options],
            ],
        )
