from __future__ import annotations
from typing import List, Tuple, Union

from ipp.base import CONTEXT, Location, Name, Type, Source, declare_variable
from ipp.prim import Array, Pointer
from ipp.util import list_join

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

    def _fields_with_comma(self) -> List[Tuple[Field, str]]:
        if len(self.fields) > 0:
            return [(f, ",") for f in self.fields[:-1]] + [(self.fields[-1], "")]
        else:
            return []

    def _c_size_func_name(self) -> str:
        return Name(CONTEXT.prefix, self.name(), "size").snake()

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

    def _cpp_size_method_lines(self) -> str:
        return [
            f"[[nodiscard]] size_t packed_size() const {{",
            f"    return {self.min_size()} + {self._cpp_size_extent('(*this)')};",
            f"}}",
        ]

    def _cpp_load_method_lines(self) -> str:
        return [
            f"[[nodiscard]] static {self.cpp_type()} load({Pointer(self, const=True).c_type()} src) {{",
            f"    return {self.cpp_type()}{{",
            *[f"        {f.type.cpp_load(f'(src->{f.name.snake()})')}," for f in self.fields],
            f"    }};",
            f"}}",
        ]

    def _cpp_store_method_lines(self) -> str:
        return [
            f"void store({Pointer(self).c_type()} dst) {{",
            *[f"    {f.type.cpp_store(f'{f.name.snake()}', f'(dst->{f.name.snake()})')};" for f in self.fields],
            f"}}",
        ]

    def _cpp_definition(self) -> str:
        sections = []

        fields_lines = [f"{f.type.cpp_type()} {f.name.snake()};" for f in self.fields]
        if len(fields_lines) > 0:
            sections.append(fields_lines)

        if not self.sized or self.size() > 0:
            sections.append([
                *self._cpp_size_method_lines(),
                *self._cpp_load_method_lines(),
                *self._cpp_store_method_lines(),
            ])
        else:
            sections.append([
                f"[[nodiscard]] size_t packed_size() const {{ return 0; }}"
            ])

        return "\n".join([
            f"class {self.cpp_type()} final {{",
            f"public:",
            *list_join([["    " + s for s in lines] for lines in sections], [""]),
            f"}};",
        ])

    def c_type(self) -> str:
        if isinstance(self._name, Name):
            return Name(CONTEXT.prefix, self.name()).camel()
        else:
            self._name

    def cpp_type(self) -> str:
        if isinstance(self._name, Name):
            return Name(self.name()).camel()
        else:
            self._name

    def c_source(self) -> Source:
        return Source(
            Location.DECLARATION,
            [
                self._c_definition() if not self.sized or self.size() > 0 else None,
                self._c_size_definition() if not self.sized else None,
            ],
            deps=[field.type.c_source() for field in self.fields],
        )

    def cpp_source(self) -> Source:
        return Source(
            Location.DECLARATION,
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

    def cpp_load(self, src: str) -> str:
        return f"{Name(self.name(), 'load').snake()}(&{src})"
    
    def cpp_store(self, src: str, dst: str) -> str:
        return f"{Name(self.name(), 'store').snake()}({src}, &{dst})"
