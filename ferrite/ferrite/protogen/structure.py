from __future__ import annotations
from typing import Any, List, Optional, Tuple

from random import Random

from ferrite.protogen.base import CONTEXT, Location, Name, Type, Value, Source, UnexpectedEof
from ferrite.protogen.utils import flatten, indent


class Field:

    def __init__(self, name: Name, type: Type):
        self.name = name
        self.type = type


class StructValue(Value):

    def __init__(self, type: Struct, fields: List[Tuple[str, Any]]):
        self._type = type
        self._fields = [v for k, v in fields]
        for k, v in fields:
            setattr(self, k, v)

    def store(self) -> bytes:
        return self._type.store(self)

    def size(self) -> int:
        return self._type.size_of(self)


class Struct(Type):

    def __init__(self, name: Name, fields: List[Field]):
        size = 0
        sized = True
        if len(fields) > 0:
            for f in fields[:-1]:
                assert f.type.is_sized()
                size += f.type.size

            f = fields[-1]
            if f.type.is_sized():
                sized = True
                size += f.type.size
            else:
                sized = False
                min_size = size + f.type.min_size
                del size

        if sized:
            super().__init__(name, size)
        else:
            super().__init__(name, None, min_size)

        self.fields = fields

    def load(self, data: bytes) -> StructValue:
        if len(data) < self.min_size:
            raise UnexpectedEof(self, data)
        args = []
        if len(self.fields) > 0:
            index = 0
            for i, f in enumerate(self.fields):
                args.append(f.type.load(data[index:]))
                if i < len(self.fields) - 1:
                    index += f.type.size
        return self.value(*args)

    def store(self, value: StructValue) -> bytes:
        assert self.is_instance(value)
        data = b""
        for f, v in zip(self.fields, value._fields):
            data += f.type.store(v)
        return data

    def size_of(self, value: StructValue) -> int:
        assert self.is_instance(value)
        if self.is_empty():
            return 0
        else:
            size = sum([f.type.size for f in self.fields[:-1]])
            size += self.fields[-1].type.size_of(value._fields[-1])
            return size

    def value(self, *args: Any) -> StructValue:
        assert len(self.fields) == len(args)
        fields = [(f.name.snake(), v) for f, v in zip(self.fields, args)]
        for f, (k, v) in zip(self.fields, fields):
            assert f.type.is_instance(v)
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

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name).camel()

    def c_size(self, obj: str) -> str:
        if self.is_sized():
            return f"((size_t){self.size})"
        else:
            return f"{self._c_size_func_name()}(&{obj})"

    def _c_size_extent(self, obj: str) -> str:
        return self.fields[-1].type._c_size_extent(f"({obj}.{self.fields[-1].name.snake()})")

    def _c_size_func_name(self) -> str:
        return Name(CONTEXT.prefix, self.name, "size").snake()

    def _c_struct_decl(self) -> List[str]:
        return [
            f"typedef struct __attribute__((packed, aligned(1))) {{",
            *[f"    {f.type.c_type()} {f.name.snake()};" for f in self.fields if not f.type.is_empty()],
            f"}} {self.c_type()};",
        ]

    def _c_size_decl(self) -> str:
        return f"size_t {self._c_size_func_name()}(const {self.c_type()} *obj)"

    def _c_size_def(self) -> List[str]:
        return [
            f"{self._c_size_decl()} {{",
            f"    return {self.min_size} + {self._c_size_extent('(*obj)')};",
            f"}}",
        ]

    def c_source(self) -> Source:
        decl_source = Source(
            Location.DECLARATION,
            [
                *([self._c_struct_decl()] if not self.is_empty() else []),
                *([[f"{self._c_size_decl()};"]] if not self.is_sized() else []),
            ],
            deps=[f.type.c_source() for f in self.fields],
        )
        return Source(
            Location.DEFINITION,
            [
                *([self._c_size_def()] if not self.is_sized() else []),
            ],
            deps=[decl_source],
        )

    def rust_type(self) -> str:
        return self.name.camel()

    def rust_source(self) -> Source:
        return Source(
            Location.DECLARATION,
            [[
                f"#[flat(portable = true, sized = {'true' if self.is_sized() else 'false'})]",
                f"pub struct {self.rust_type()} {{",
                *indent([f"pub {f.name.snake()}: {f.type.rust_type()}," for f in self.fields]),
                f"}}",
            ]],
            deps=[f.type.rust_source() for f in self.fields],
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
                *([f"    {f.name.snake()}: {f.type.pyi_type()}"
                   for f in self.fields] if len(self.fields) > 0 else ["    pass"]),
            ]],
            deps=[
                Source(Location.IMPORT, [["from dataclasses import dataclass"]]),
                *[f.type.pyi_source() for f in self.fields],
            ],
        )

    def c_check(self, var: str, obj: StructValue) -> List[str]:
        return flatten([f.type.c_check(f"{var}.{f.name.snake()}", getattr(obj, f.name.snake())) for f in self.fields])

    def rust_check(self, var: str, obj: StructValue) -> List[str]:
        return flatten([f.type.rust_check(f"(&{var}.{f.name.snake()})", getattr(obj, f.name.snake())) for f in self.fields])

    def rust_object(self, obj: StructValue) -> str:
        return f"{self.name.camel()} {{" + ", ".join([getattr(obj, f.name.snake()) for f in self.fields]) + f"}}"
