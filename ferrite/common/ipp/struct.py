from __future__ import annotations
from typing import List, Union, overload
from dataclasses import dataclass

from ipp.base import Name, Type, Source, declare_variable
from ipp.prim import Array, Int, Pointer, Reference
from ipp.util import ceil_to_power_of_2

class Field:
    def __init__(self, name: Union[Name, str], type: Type):
        self.name = Name(name)
        self.type = type

def should_ignore(type: Type) -> bool:
    if not type.sized:
        return False
    if type.size() > 0:
        return False
    if isinstance(type, Array) and type.len == 0 and type.type.size() > 0:
        return False
    return True

class Struct(Type):
    def __init__(self, name: Union[Name, str], fields: List[Field] = []):
        sized = True
        if len(fields) > 0:
            for f in fields[:-1]:
                assert f.type.sized
            sized = fields[-1].type.sized
        super().__init__(sized=sized)
        self._name = name
        self.fields = fields

    def name(self) -> Name:
        return Name(self._name)

    def min_size(self) -> int:
        if len(self.fields) > 0:
            return sum([f.type.size() for f in self.fields[:-1]]) + self.fields[-1].type.min_size()
        else:
            return 0

    def size(self) -> int:
        return sum([f.type.size() for f in self.fields])

    def _c_size_func_name(self) -> str:
        return Name(self.name(), "size").snake()

    def _c_size_extent(self, obj: str) -> str:
        return self.fields[-1].type._c_size_extent(f"({obj}.{self.fields[-1].name.snake()})")
    
    def _cpp_size_extent(self, obj: str) -> str:
        return self.fields[-1].type._cpp_size_extent(f"({obj}.{self.fields[-1].name.snake()})")

    def _c_definition(self) -> str:
        return "\n".join([
            f"typedef struct __attribute__((packed, aligned(1))) {self.c_type()} {{",
            *[
                f"    {declare_variable(f.type.c_type(), f.name.snake())};"
                for f in self.fields
                if not should_ignore(f.type)
            ],
            f"}} {self.c_type()};",
        ])

    def _c_size_definition(self) -> str:
        return "\n".join([
            f"size_t {self._c_size_func_name()}({Pointer(self, const=True).c_type()} obj) {{",
            f"    return {self.min_size()} + {self._c_size_extent('(*obj)')};",
            f"}}",
        ])

    def _cpp_definition(self) -> str:
        return "\n".join([
            f"class {self.cpp_type()} final {{",
            f"public:",
            *[f"    {f.type.cpp_type()} {f.name.snake()};" for f in self.fields],
            f"",
            *([
                f"    [[nodiscard]] size_t packed_size() const {{",
                f"        return {self.min_size()} + {self._cpp_size_extent('(*this)')};",
                f"    }}",
            ] if not self.sized or self.size() > 0 else [
                f"    [[nodiscard]] size_t packed_size() const {{ return 0; }}",
            ]),
            f"}};",
        ])

    def c_type(self) -> str:
        return self.name().camel() if isinstance(self._name, Name) else self._name

    def c_source(self) -> Source:
        return Source(
            [
                self._c_definition() if not self.sized or self.size() > 0 else None,
                self._c_size_definition() if not self.sized else None,
            ],
            deps=[field.type.c_source() for field in self.fields],
        )

    def cpp_source(self) -> Source:
        return Source(
            [self._cpp_definition()],
            [field.type.cpp_source() for field in self.fields],
        )

    def c_size(self, obj: str) -> str:
        if self.sized:
            return str(self.size())
        else:
            return f"{self._c_size_func_name()}(&{obj})"

    def cpp_size(self, obj: str) -> str:
        return f"({obj}.size() * {self.item.size()})"

    def cpp_load(self, dst: str, src: str) -> str:
        return f"{dst} = {Name(self.name(), 'load').snake()}(&{src})"
    
    def cpp_store(self, src: str, dst: str) -> str:
        return f"{Name(self.name(), 'store').snake()}({src}, &{dst})"

class Variant(Type):
    def __init__(self, name: Union[Name, str], options: List[Field]):
        super().__init__(sized=all([f.type.sized for f in options]))
        self._name = name
        self.options = options
        self._id_type = Int(max(8, ceil_to_power_of_2(len(self.options))))

    def name(self):
        return Name(self._name)

    def min_size(self) -> int:
        return max([f.type.min_size() for f in self.options]) + self._id_type.size()

    def size(self) -> int:
        return max([f.type.size() for f in self.options]) + self._id_type.size()

    def _c_enum_type(self) -> str:
        return Name(self.name(), "type").camel()

    def _c_enum_definition(self) -> str:
        return "\n".join([
            f"typedef enum {self._c_enum_type()} {{",
            *[f"    {Name(self.name(), f.name).snake().upper()} = {i}," for i, f in enumerate(self.options)],
            f"}} {self._c_enum_type()};",
        ])

    def _c_struct_definition(self) -> str:
        return "\n".join([
            f"typedef struct __attribute__((packed, aligned(1))) {self.c_type()} {{",
            f"    {self._id_type.c_type()} type;",
            f"    union {{",
            *[
                f"        {f.type.c_type()} {f.name.snake()};"
                for f in self.options
                if not should_ignore(f.type)
            ],
            f"    }};",
            f"}} {self.c_type()};",
        ])

    def _c_size_definition(self) -> str:
        return "\n".join([
            f"size_t {Name(self.name(), 'size').snake()}({Pointer(self, const=True).c_type()} obj) {{",
            f"    size_t size = {self._id_type.size()};",
            f"    switch (({self._c_enum_type()})obj->type) {{",
            *[
                "\n".join([
                    f"    case {Name(self.name(), f.name).snake().upper()}:",
                    f"        size += {f.type.c_size(f'(obj->{f.name.snake()})')};",
                    f"        break;",
                ])
                for i, f in enumerate(self.options)
            ],
            f"    }}",
            f"    return size;",
            f"}}",
        ])

    def _cpp_definition(self) -> str:
        return "\n".join([
            f"class {self.cpp_type()} final {{",
            f"public:",
            f"    std::variant<",
            *[
                f"        {option.type.cpp_type()}{',' if i < len(self.options) else ''}"
                for i, option in enumerate(self.options)
            ],
            f"    > variant;",
            f"",
            f"    [[nodiscard]] size_t packed_size() const {{",
            f"        return {self._id_type.size()} + std::visit([](const auto &v) {{",
            f"            return v.packed_size();",
            f"        }}, variant);",
            f"    }}",
            f"}};",
        ])

    def c_type(self) -> str:
        return self.name().camel()

    def cpp_type(self) -> str:
        return self.name().camel()

    def c_source(self) -> Source:
        return Source(
            [
                self._c_enum_definition(),
                self._c_struct_definition(),
                self._c_size_definition(),
            ],
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


    def c_size(self, obj: str) -> str:
        if self.sized:
            return str(self.size())
        else:
            return f"{self._c_size_func_name()}(&{obj})"

    def cpp_size(self, obj: str) -> str:
        return f"({obj}.size() * {self.item.size()})"

    def cpp_load(self, dst: str, src: str) -> str:
        return f"{dst} = {Name(self.name(), 'load').snake()}(&{src})"
    
    def cpp_store(self, src: str, dst: str) -> str:
        return f"{Name(self.name(), 'store').snake()}({src}, &{dst})"