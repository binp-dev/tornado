from __future__ import annotations
from typing import Any, Dict, List, Optional, Tuple, Union

from random import Random

from ferrite.codegen.base import CONTEXT, Location, Name, Type, Source, declare_variable
from ferrite.codegen.primitive import Pointer
from ferrite.codegen.utils import indent, list_join


class Field:

    def __init__(self, name: Union[Name, str], type: Type):
        self.name = Name(name)
        self.type = type


class StructValue:

    def __init__(self, type: Struct, fields: Dict[str, Any]):
        self._type = type
        for k, v in fields.items():
            setattr(self, k, v)

    def store(self) -> bytes:
        return self._type.store(self)


class Struct(Type):

    def __init__(self, name: Name, fields: List[Field] = []):
        sized = True
        if len(fields) > 0:
            for f in fields[:-1]:
                assert f.type.sized
            sized = fields[-1].type.sized
        super().__init__(sized=sized)
        self._name = name
        self.fields = fields

    def name(self) -> Name:
        return self._name

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
        return value._type is self

    def deps(self) -> List[Type]:
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

    def _c_struct_declaraion(self) -> List[str]:
        return [
            f"typedef struct {{",
            *[f"    {declare_variable(f.type.c_type(), f.name.snake())};" for f in self.fields if not f.type.is_empty()],
            f"}} {self.c_type()};",
        ]

    def _c_size_decl(self) -> str:
        return f"size_t {self._c_size_func_name()}({Pointer(self, const=True).c_type()} obj)"

    def _c_size_definition(self) -> List[str]:
        return [
            f"{self._c_size_decl()} {{",
            f"    return {self.min_size()} + {self._c_size_extent('(*obj)')};",
            f"}}",
        ]

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name()).camel()

    def rust_type(self) -> str:
        return self.name().camel()

    def c_source(self) -> Source:
        decl_source = Source(
            Location.DECLARATION,
            [
                *([self._c_struct_declaraion()] if not self.is_empty() else []),
                *([[f"{self._c_size_decl()};"]] if not self.sized else []),
            ],
            deps=[ty.c_source() for ty in self.deps()],
        )
        return Source(
            Location.DEFINITION,
            [
                *([self._c_size_definition()] if not self.sized else []),
            ],
            deps=[decl_source],
        )

    def rust_source(self) -> Source:
        return Source(
            Location.DECLARATION,
            [[
                f"#[make_flat(sized = {'true' if self.sized else 'false'})]",
                f"pub struct {self.rust_type()} {{",
                *indent([f"pub {f.name.snake()}: {f.type.rust_type()}," for f in self.fields]),
                f"}}",
            ]],
            deps=[
                Source(Location.INCLUDES, [["use flatty::make_flat;"]]),
                *[ty.rust_source() for ty in self.deps()],
            ],
        )

    def c_size(self, obj: str) -> str:
        if self.sized:
            return f"((size_t){self.size()})"
        else:
            return f"{self._c_size_func_name()}(&{obj})"

    def pyi_type(self) -> str:
        return self.name().camel()

    def pyi_source(self) -> Optional[Source]:
        return Source(
            Location.DECLARATION,
            [[
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
            ]],
            deps=[
                Source(Location.INCLUDES, [["from dataclasses import dataclass"]]),
                *[ty.pyi_source() for ty in self.deps()],
            ],
        )
