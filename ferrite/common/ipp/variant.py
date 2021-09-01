from __future__ import annotations
from typing import List, Tuple, Union

from ipp.base import CONTEXT, Include, Location, Name, Type, Source
from ipp.prim import Int, Pointer
from ipp.util import ceil_to_power_of_2
from ipp.struct import Field, should_ignore


class Variant(Type):
    def __init__(self, name: Union[Name, str], options: List[Field]):
        super().__init__(sized=all([f.type.sized for f in options]))
        self._name = name
        self.options = options
        self._id_type = Int(max(8, ceil_to_power_of_2(len(self.options))))

    def _options_with_comma(self) -> List[Tuple[Field, str]]:
        if len(self.options) > 0:
            return [(f, ",") for f in self.options[:-1]] + [(self.options[-1], "")]
        else:
            return []

    def name(self):
        return Name(self._name)

    def min_size(self) -> int:
        return max([f.type.min_size() for f in self.options]) + self._id_type.size()

    def size(self) -> int:
        return max([f.type.size() for f in self.options]) + self._id_type.size()

    def _c_enum_type(self) -> str:
        return Name(CONTEXT.prefix, self.name(), "type").camel()

    def _c_enum_definition(self) -> str:
        return "\n".join([
            f"typedef enum {self._c_enum_type()} {{",
            *[f"    {Name(CONTEXT.prefix, self.name(), f.name).snake().upper()} = {i}," for i, f in enumerate(self.options)],
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
            f"size_t {Name(CONTEXT.prefix, self.name(), 'size').snake()}({Pointer(self, const=True).c_type()} obj) {{",
            f"    size_t size = {self._id_type.size()};",
            f"    switch (({self._c_enum_type()})obj->type) {{",
            *[
                "\n".join([
                    f"    case {Name(CONTEXT.prefix, self.name(), f.name).snake().upper()}:",
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
                f"        {option.type.cpp_type()}{c}"
                for option, c in self._options_with_comma()
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
        return Name(CONTEXT.prefix, self.name()).camel()

    def cpp_type(self) -> str:
        return self.name().camel()

    def c_source(self) -> Source:
        return Source(
            Location.DECLARATION,
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
            Location.DECLARATION,
            [self._cpp_definition()],
            [
                Include("variant"),
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

    def cpp_load(self, src: str) -> str:
        return f"{Name(self.name(), 'load').snake()}(&{src})"
    
    def cpp_store(self, src: str, dst: str) -> str:
        return f"{Name(self.name(), 'store').snake()}({src}, &{dst})"
