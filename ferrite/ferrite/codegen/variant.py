from __future__ import annotations
from typing import Any, List, Optional, Tuple, Union

from random import Random

from ferrite.codegen.base import CONTEXT, Location, Name, Type, Value, Source, UnexpectedEof
from ferrite.codegen.primitive import Int
from ferrite.codegen.utils import indent, flatten, pad_bytes
from ferrite.codegen.structure import Field


class VariantValue(Value):

    def __init__(self, type: Variant, id: int, variant: Any):
        self._type = type
        self.id = id
        self.variant = variant

    def variant_type(self) -> Type:
        return self._type.variants[self.id].type

    def store(self) -> bytes:
        return self._type.store(self)

    def size(self) -> int:
        return self._type.size_of(self)


class Variant(Type):

    def __init__(self, name: Name, variants: List[Field], sized: bool | None = None):
        id_type = Int(8)
        assert len(variants) < (1 << id_type.bits)

        all_variants_sized = all([f.type.is_sized() for f in variants])
        if sized is None:
            sized = all_variants_sized
        elif sized is True:
            assert all_variants_sized

        size = id_type.size
        if sized:
            size += max([f.type.size for f in variants])
            super().__init__(name, size)
        else:
            min_size = size + min([f.type.min_size for f in variants])
            del size
            super().__init__(name, None, min_size)

        self._id_type = Int(8)
        self.variants = variants

        # Variant types for convenience
        for f in self.variants:
            setattr(self, f.name.camel(), f.type)

    def load(self, data: bytes) -> VariantValue:
        if len(data) < self.min_size:
            raise UnexpectedEof(self, data)

        id = self._id_type.load(data[:self._id_type.size])

        var_data = data[self._id_type.size:]
        var_type = self.variants[id].type
        variant = var_type.load(var_data)

        return self.value(id, variant)

    def store(self, value: VariantValue) -> bytes:
        data = self._id_type.store(value.id)
        data += value.variant_type().store(value.variant)

        if self.is_sized():
            assert self.size >= len(data)
            data = pad_bytes(data, self.size)

        return data

    def size_of(self, value: VariantValue) -> int:
        return self._id_type.size + value.variant_type().size_of(value.variant)

    def value(self, id: int, value: Any) -> VariantValue:
        assert self.variants[id].type.is_instance(value)
        return VariantValue(self, id, value)

    def __call__(self, variant: Any) -> Any:
        value = None
        for i, f in enumerate(self.variants):
            if f.type.is_instance(variant):
                assert value is None, f"{type(variant).__name__} is ambiguous. Use `value` method instead to provide variant index."
                value = self.value(i, variant)
        assert value is not None, f"{type(variant).__name__} is not an instance of any variant."
        return value

    def random(self, rng: Random) -> VariantValue:
        id = rng.randrange(0, len(self.variants))
        variant = self.variants[id].type.random(rng)
        return self.value(id, variant)

    def is_instance(self, value: VariantValue) -> bool:
        return value._type is self

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name).camel()

    def c_size(self, obj: str) -> str:
        if self.is_sized():
            return str(self.size)
        else:
            return f"{self._c_size_func_name()}(&{obj})"

    def _c_size_extent(self, obj: str) -> str:
        return f"({self.c_size(obj)} - {self.min_size})"

    def _c_size_func_name(self) -> str:
        return Name(CONTEXT.prefix, self.name, "size").snake()

    def _c_enum_type(self) -> str:
        return Name(CONTEXT.prefix, self.name, "type").camel()

    def _c_enum_value(self, index: int) -> str:
        return Name(CONTEXT.prefix, self.name, self.variants[index].name).snake().upper()

    def _c_enum_decl(self) -> List[str]:
        return [
            f"typedef enum {{",
            *[f"    {self._c_enum_value(i)} = {i}," for i, f in enumerate(self.variants)],
            f"}} {self._c_enum_type()};",
        ]

    def _c_struct_decl(self) -> List[str]:
        return [
            f"typedef struct __attribute__((packed, aligned(1))) {{",
            f"    {self._id_type.c_type()} type;",
            f"    union {{",
            *[f"        {f.type.c_type()} {f.name.snake()};" for f in self.variants if not f.type.is_empty()],
            f"    }};",
            f"}} {self.c_type()};",
        ]

    def _c_size_decl(self) -> str:
        return f"size_t {self._c_size_func_name()}(const {self.c_type()} *obj)"

    def _c_size_def(self) -> List[str]:
        return [
            f"{self._c_size_decl()} {{",
            f"    size_t size = {self._id_type.size};",
            f"    switch (({self._c_enum_type()})(obj->type)) {{",
            *flatten([[
                f"    case {self._c_enum_value(i)}:",
                f"        size += {f.type.c_size(f'(obj->{f.name.snake()})')};",
                f"        break;",
            ] for i, f in enumerate(self.variants)]),
            f"    default:",
            f"        abort(); // unreachable",
            f"    }}",
            f"    return size;",
            f"}}",
        ]

    def c_source(self) -> Source:
        decl_source = Source(
            Location.DECLARATION,
            [
                self._c_enum_decl(),
                self._c_struct_decl(),
                *([[f"{self._c_size_decl()};"]] if not self.is_sized() else []),
            ],
            deps=[
                self._id_type.c_source(),
                *[f.type.c_source() for f in self.variants],
            ],
        )
        return Source(
            Location.DEFINITION,
            [self._c_size_def()] if not self.is_sized() else [],
            deps=[decl_source],
        )

    def rust_type(self) -> str:
        return self.name.camel()

    def rust_source(self) -> Source:
        return Source(
            Location.DECLARATION,
            [[
                f"#[make_flat(",
                *indent([
                    f"portable = true,",
                    f"sized = {'true' if self.is_sized() else 'false'},",
                    f"enum_type = \"{self._id_type.rust_type()}\",",
                    f"default = {'true' if CONTEXT.default else 'false'},",
                ]),
                f")]",
                f"pub enum {self.rust_type()} {{",
                *indent(([f"#[default]"] if CONTEXT.default else []) +
                        [f"{f.name.camel()}({f.type.rust_type()})," for f in self.variants]),
                f"}}",
            ]],
            deps=[f.type.rust_source() for f in self.variants],
        )

    def pyi_type(self) -> str:
        return self.name.camel()

    def pyi_source(self) -> Optional[Source]:
        return Source(
            Location.DECLARATION,
            [[
                f"@dataclass",
                f"class {self.pyi_type()}(Value):",
                f"",
                f"    Variant = {' | '.join([f.type.pyi_type() for f in self.variants])}",
                f"",
                *[f"    {f.name.camel()} = {f.type.pyi_type()}" for f in self.variants],
                f"",
                f"    variant: Variant",
            ]],
            deps=[
                Source(Location.IMPORT, [["from dataclasses import dataclass"]]),
                *[f.type.pyi_source() for f in self.variants],
            ],
        )

    def c_check(self, var: str, obj: VariantValue) -> List[str]:
        return [
            f"codegen_assert_eq(({self._c_enum_type()}){var}.type, {self._c_enum_value(obj.id)});",
            *obj.variant_type().c_check(f'{var}.{self.variants[obj.id].name.snake()}', obj.variant),
        ]

    def rust_check(self, var: str, obj: VariantValue) -> List[str]:
        if self.is_sized():
            return [
                f"if let {self.rust_type()}::{self.variants[obj.id].name.camel()}(tmp) = {var} {{",
                *indent(obj.variant_type().rust_check("tmp", obj.variant)),
                f"}}",
            ]
        else:
            return [
                f"if let {self.rust_type()}Ref::{self.variants[obj.id].name.camel()}(tmp) = {var}.as_ref() {{",
                *indent(obj.variant_type().rust_check("tmp", obj.variant)),
                f"}}",
            ]

    def rust_object(self, obj: VariantValue) -> str:
        return f"{self.name.camel()}::{obj.variant_type().name.camel()} (" + obj.variant_type().rust_object(obj) + f")"
