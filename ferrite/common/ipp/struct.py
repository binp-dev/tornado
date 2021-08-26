from __future__ import annotations
from typing import List
from dataclasses import dataclass

from ipp.base import Type, Prelude

@dataclass
class Field:
    name: str
    type: Type

class Struct(Type):
    def __init__(self, name: str, fields: List[Field] = []):
        self.name = name
        self.fields = fields

    def _c_definition(self) -> str:
        return "".join([s + "\n" for s in [
            f"typedef struct {self.name} {{",
            *[f"    {field.type.c_type()} {field.name};" for field in self.fields],
            f"}} {self.name};"
        ]])

    def _cpp_definition(self) -> str:
        return "".join([s + "\n" for s in [
            f"class {self.name} final {{",
            f"private:",
            *[f"    {field.type.cpp_type()} {field.name}_;" for field in self.fields],
            f"}};"
        ]])

    def c_type(self) -> str:
        return self.name
    
    def cpp_type(self) -> str:
        return self.name

    def c_prelude(self) -> Prelude:
        return Prelude(
            self._c_definition(),
            [p for p in [field.type.c_prelude() for field in self.fields] if p is not None],
        )
    
    def cpp_prelude(self) -> Prelude:
        return Prelude(
            self._cpp_definition(),
            [p for p in [field.type.cpp_prelude() for field in self.fields] if p is not None],
        )

class Variant(Type):
    def __init__(self, name: str, options: List[Field]):
        self.name = name
        self.options = options

    def _c_definition(self) -> str:
        return "".join([s + "\n" for s in [
            f"typedef struct {self.name} {{",
            f"    uint8_t type;",
            f"    union {{",
            *[f"        {option.type.c_type()} {option.name};" for option in self.options],
            f"    }};"
            f"}} {self.name};"
        ]])

    def _cpp_definition(self) -> str:
        return "".join([s + "\n" for s in [
            f"class {self.name} final {{",
            f"private:",
            f"    std::variant<",
            *[
                f"        {option.type.cpp_type()}{',' if i < len(self.options) else ''}"
                for i, option in enumerate(self.options)
            ],
            f"    > variant_;",
            f"}};"
        ]])

    def c_type(self) -> str:
        return self.name

    def cpp_type(self) -> str:
        return self.name

    def c_prelude(self) -> Prelude:
        return Prelude(
            self._c_definition(),
            [
                Prelude("#include <stdint.h>"),
                *[p for p in [option.type.c_prelude() for option in self.options] if p is not None],
            ],
        )

    def cpp_prelude(self) -> Prelude:
        return Prelude(
            self._cpp_definition(),
            [
                Prelude("#include <variant>"),
                *[p for p in [option.type.cpp_prelude() for option in self.options] if p is not None],
            ],
        )
