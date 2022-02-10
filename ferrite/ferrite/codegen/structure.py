from __future__ import annotations
from typing import Any, Dict, Generic, List, Optional, Tuple, TypeVar, Union

from random import Random

from ferrite.codegen.base import CONTEXT, Location, Name, Type, Source, declare_variable
from ferrite.codegen.primitive import Pointer
from ferrite.codegen.utils import indent_text, list_join


class Field:

    def __init__(self, name: Union[Name, str], type: Type[Any]):
        self.name = Name(name)
        self.type = type


class StructValue:

    def __init__(self, type: Struct, fields: Dict[str, Any]):
        self._type = type
        for k, v in fields.items():
            setattr(self, k, v)

    def store(self) -> bytes:
        return self._type.store(self)


class Struct(Type[StructValue]):

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

    def load(self, data: bytes) -> StructValue:
        args = []
        next_data: Optional[bytes] = data
        for f in self.fields:
            ty = f.type
            assert next_data is not None
            args.append(ty.load(next_data))
            next_data = next_data[ty.size():] if ty.sized else None
        return self.value(*args)

    def store(self, value: StructValue) -> bytes:
        self.is_instance(value)
        data = b""
        for f in self.fields:
            k = f.name.snake()
            data += f.type.store(getattr(value, k))
        return data

    def value(self, *args: Any, **kwargs: Any) -> StructValue:
        fields = {}
        field_types = {f.name.snake(): f.type for f in self.fields}
        for k, v in zip(field_types, args):
            fields[k] = v
        for k, v in kwargs.items():
            assert k in field_types
            assert k not in fields
            fields[k] = v
        assert len(self.fields) == len(fields)
        for k, v in fields.items():
            assert field_types[k].is_instance(v)
        return StructValue(self, fields)

    def __call__(self, *args: Any, **kwargs: Any) -> Any:
        return self.value(*args, **kwargs)

    def random(self, rng: Random) -> StructValue:
        args = []
        for f in self.fields:
            args.append(f.type.random(rng))
        return self.value(*args)

    def is_instance(self, value: StructValue) -> bool:
        return isinstance(value, StructValue) and value._type == self

    def deps(self) -> List[Type[Any]]:
        return [f.type for f in self.fields]

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

    def _c_struct_declaraion(self) -> str:
        return "\n".join([
            f"typedef struct __attribute__((packed, aligned(1))) {{",
            *[f"    {declare_variable(f.type.c_type(), f.name.snake())};" for f in self.fields if not f.type.is_empty()],
            f"}} {self.c_type()};",
        ])

    def _c_size_decl(self) -> str:
        return f"size_t {self._c_size_func_name()}({Pointer(self, const=True).c_type()} obj)"

    def _c_size_definition(self) -> str:
        return "\n".join([
            f"{self._c_size_decl()} {{",
            f"    return {self.min_size()} + {self._c_size_extent('(*obj)')};",
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
            (
                f"    return {self.min_size()} + {self._cpp_size_extent('(*this)')};"
                if not self.sized else f"    return {self.size()};"
            ),
            f"}}",
        ])

    def _cpp_load_method_impl(self) -> str:
        return "\n".join([
            f"{self.cpp_type()} {self.cpp_type()}::load({Pointer(self, const=True).c_type()} src) {{",
            f"    return {self.cpp_type()}{{",
            *[f"        {f.type.cpp_load(f'(src->{f.name.snake()})')}," for f in self.fields],
            f"    }};",
            f"}}",
        ])

    def _cpp_store_method_impl(self) -> str:
        return "\n".join([
            f"void {self.cpp_type()}::store({Pointer(self).c_type()} dst) const {{",
            *[f"    {f.type.cpp_store(f'{f.name.snake()}', f'(dst->{f.name.snake()})')};" for f in self.fields],
            f"}}",
        ])

    def _cpp_declaration(self) -> Source:
        sections = []

        fields_lines = [f"{f.type.cpp_type()} {f.name.snake()};" for f in self.fields]
        if len(fields_lines) > 0:
            sections.append(fields_lines)

        methods = [
            self._cpp_size_method_decl(),
        ]
        if not self.is_empty():
            methods.extend([
                self._cpp_load_method_decl(),
                self._cpp_store_method_decl(),
            ])
        sections.append(methods)

        return Source(
            Location.DECLARATION,
            "\n".join([
                f"class {self.cpp_type()} final {{",
                f"public:",
                *([f"    using Raw = {self.c_type()};"] if not self.is_empty() else []),
                *list_join([["    " + s for s in lines] for lines in sections], [""]),
                f"}};",
            ]),
            deps=[ty.cpp_source() for ty in self.deps()],
        )

    def _cpp_definition(self) -> Source:
        items = [
            self._cpp_size_method_impl(),
        ]
        if not self.is_empty():
            items.extend([
                self._cpp_load_method_impl(),
                self._cpp_store_method_impl(),
            ])

        return Source(
            Location.DEFINITION,
            items,
            deps=[self._cpp_declaration()],
        )

    def c_type(self) -> str:
        if isinstance(self._name, Name):
            return Name(CONTEXT.prefix, self.name()).camel()
        else:
            return self._name

    def cpp_type(self) -> str:
        if isinstance(self._name, Name):
            return Name(self.name()).camel()
        else:
            return self._name

    def c_source(self) -> Source:
        decl_source = Source(
            Location.DECLARATION,
            [
                self._c_struct_declaraion() if not self.is_empty() else None,
                f"{self._c_size_decl()};" if not self.sized else None,
            ],
            deps=[ty.c_source() for ty in self.deps()],
        )
        return Source(
            Location.DEFINITION,
            [
                self._c_size_definition() if not self.sized else None,
            ],
            deps=[decl_source],
        )

    def cpp_source(self) -> Source:
        return self._cpp_definition()

    def c_size(self, obj: str) -> str:
        if self.sized:
            return f"((size_t){self.size()})"
        else:
            return f"{self._c_size_func_name()}(&{obj})"

    def cpp_size(self, obj: str) -> str:
        return f"{obj}.packed_size()"

    def cpp_load(self, src: str) -> str:
        return f"{self.cpp_type()}::load(&{src})"

    def cpp_store(self, src: str, dst: str) -> str:
        return f"{src}.store(&{dst})"

    def cpp_object(self, value: StructValue) -> str:
        return "\n".join([
            f"{self.cpp_type()}{{",
            *[indent_text(f"{f.type.cpp_object(getattr(value, f.name.snake()))},", "    ") for f in self.fields],
            f"}}",
        ])

    def c_test(self, obj: str, src: str) -> str:
        lines = []
        for f in self.fields:
            fname = f.name.snake()
            if not f.type.is_empty():
                lines.append(f.type.c_test(f"{obj}.{fname}", f"{src}.{fname}"))
        return "\n".join(lines)

    def cpp_test(self, dst: str, src: str) -> str:
        lines = []
        for f in self.fields:
            fname = f.name.snake()
            lines.append(f.type.cpp_test(f"{dst}.{fname}", f"{src}.{fname}"))
        return "\n".join(lines)

    def test_source(self) -> Optional[Source]:
        if not self.is_empty():
            return super().test_source()
        else:
            return None

    def pyi_type(self) -> str:
        return self.cpp_type()

    def pyi_source(self) -> Optional[Source]:
        return Source(
            Location.DECLARATION,
            "\n".join([
                f"@dataclass",
                f"class {self.pyi_type()}:",
                f"",
                *[f"    {f.name.snake()}: {f.type.pyi_type()}" for f in self.fields],
                f"",
                f"    @staticmethod",
                f"    def load(data: bytes) -> {self.pyi_type()}:",
                f"        ...",
                f"",
                f"    def store(self) -> bytes:",
                f"        ...",
            ]),
            deps=[
                Source(Location.INCLUDES, ["from dataclasses import dataclass"]),
                *[ty.pyi_source() for ty in self.deps()],
            ],
        )
