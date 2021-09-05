from __future__ import annotations
from random import Random
from typing import List, Tuple, Union

from ipp.base import CONTEXT, Include, Location, Name, Type, Source
from ipp.prim import Int, Pointer
from ipp.util import ceil_to_power_of_2, list_join
from ipp.struct import Field


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

    def deps(self) -> List[Type]:
        return [f.type for f in self.options]

    def _c_enum_type(self) -> str:
        return Name(CONTEXT.prefix, self.name(), "type").camel()

    def _c_enum_value(self, index: int) -> str:
        return Name(CONTEXT.prefix, self.name(), self.options[index].name).snake().upper()

    def _c_enum_declaration(self) -> str:
        return "\n".join([
            f"typedef enum {self._c_enum_type()} {{",
            *[f"    {self._c_enum_value(i)} = {i}," for i, f in enumerate(self.options)],
            f"}} {self._c_enum_type()};",
        ])

    def _c_struct_declaration(self) -> str:
        return "\n".join([
            f"typedef struct __attribute__((packed, aligned(1))) {{",
            f"    {self._id_type.c_type()} type;",
            f"    union {{",
            *[
                f"        {f.type.c_type()} {f.name.snake()};"
                for f in self.options
                if not f.type.is_empty()
            ],
            f"    }};",
            f"}} {self.c_type()};",
        ])

    def _c_size_decl(self) -> str:
        return f"size_t {Name(CONTEXT.prefix, self.name(), 'size').snake()}({Pointer(self, const=True).c_type()} obj)"

    def _c_size_definition(self) -> str:
        return "\n".join([
            f"{self._c_size_decl()} {{",
            f"    size_t size = {self._id_type.size()};",
            f"    switch (({self._c_enum_type()})(obj->type)) {{",
            *list_join([
                [
                    f"    case {self._c_enum_value(i)}:",
                    f"        size += {f.type.c_size(f'(obj->{f.name.snake()})')};",
                    f"        break;",
                ]
                for i, f in enumerate(self.options)
            ]),
            f"    }}",
            f"    return size;",
            f"}}",
        ])

    def _cpp_size_method_decl(self) -> str:
        return f"[[nodiscard]] size_t packed_size() const;"

    def _cpp_load_method_decl(self) -> str:
        return f"[[nodiscard]] static {self.cpp_type()} load({Pointer(self, const=True).c_type()} src);"

    def _cpp_store_method_decl(self) -> str:
        return f"void store({Pointer(self).c_type()} dst) const;"

    def _cpp_size_method_impl(self) -> str:
        return "\n".join([
            f"size_t {self.cpp_type()}::packed_size() const {{",
            f"    return {self._id_type.size()} + std::visit([](const auto &v) {{",
            f"        return v.packed_size();",
            f"    }}, variant);",
            f"}}",
        ])

    def _cpp_load_method_impl(self) -> str:
        return "\n".join([
            f"{self.cpp_type()} {self.cpp_type()}::load({Pointer(self, const=True).c_type()} src) {{",
            f"    switch (({self._c_enum_type()})(src->type)) {{",
            *list_join([
                [
                    f"    case {self._c_enum_value(i)}:",
                    (
                        f"        return {self.cpp_type()}{{{f.type.cpp_load(f'(src->{f.name.snake()})')}}};"
                        if not f.type.is_empty() else
                        f"        return {self.cpp_type()}{{{f.type.cpp_type()}{{}}}};"
                    ),
                ]
                for i, f in enumerate(self.options)
            ]),
            f"    }}",
            f"    std::abort();",
            f"}}",
        ])

    def _cpp_store_method_impl(self) -> str:
        return "\n".join([
            f"void {self.cpp_type()}::store({Pointer(self).c_type()} dst) const {{",
            f"    const auto type = static_cast<{self._id_type.c_type()}>(variant.index());",
            f"    dst->type = type;",
            f"    switch (type) {{",
            *list_join([
                [
                    f"    case {self._c_enum_value(i)}:",
                    *(
                        [f"        {f.type.cpp_store(f'std::get<{i}>(variant)', f'(dst->{f.name.snake()})')};"]
                        if not f.type.is_empty() else []
                    ),
                    f"        break;",
                ]
                for i, f in enumerate(self.options)
            ]),
            f"    }}",
            f"}}",
        ])

    def _cpp_declaration(self) -> Source:
        sections = []

        sections.append([
            f"std::variant<",
            *[f"    {option.type.cpp_type()}{c}" for option, c in self._options_with_comma()],
            f"> variant;",
        ])

        sections.append([
            self._cpp_size_method_decl(),
            self._cpp_load_method_decl(),
            self._cpp_store_method_decl(),
        ])

        return Source(
            Location.DECLARATION,
            "\n".join([
                f"class {self.cpp_type()} final {{",
                f"public:",
                *list_join([["    " + s for s in lines] for lines in sections], [""]),
                f"}};",
            ]),
            deps=[
                Include("variant"),
                *[ty.cpp_source() for ty in self.deps()],
            ],
        )

    def _cpp_definition(self) -> Source:
        items = []

        if not self.is_empty():
            items.extend([
                self._cpp_size_method_impl(),
                self._cpp_load_method_impl(),
                self._cpp_store_method_impl(),
            ])

        return Source(
            Location.DEFINITION,
            items,
            deps=[self._cpp_declaration()],
        )

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name()).camel()

    def cpp_type(self) -> str:
        return self.name().camel()

    def c_source(self) -> Source:
        decl_source = Source(
            Location.DECLARATION,
            [
                self._c_enum_declaration(),
                self._c_struct_declaration(),
                f"{self._c_size_decl()};",
            ],
            deps=[
                self._id_type.c_source(),
                *[ty.c_source() for ty in self.deps()],
            ],
        )
        return Source(
            Location.DEFINITION,
            [
                self._c_size_definition(),
            ],
            deps=[decl_source],
        )

    def cpp_source(self) -> Source:
        return self._cpp_definition()

    def c_size(self, obj: str) -> str:
        if self.sized:
            return str(self.size())
        else:
            return f"{self._c_size_func_name()}(&{obj})"

    def cpp_size(self, obj: str) -> str:
        return f"{obj}.packed_size()"

    def cpp_load(self, src: str) -> str:
        return f"{self.cpp_type()}::load(&{src})"

    def cpp_store(self, src: str, dst: str) -> str:
        return f"{src}.store(&{dst})"

    def test_source(self) -> Source:
        return Source(
            Location.TESTS,
            deps=[ty.test_source() for ty in self.deps()],
        )
