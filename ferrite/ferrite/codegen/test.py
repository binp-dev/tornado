from __future__ import annotations
from typing import List

from ferrite.codegen.variant import Variant
from ferrite.codegen.base import Name, Type
from ferrite.codegen.primitive import Float, Int
from ferrite.codegen.container import Array, Vector, String
from ferrite.codegen.structure import Field, Struct


class AllDict(dict[str, Type]):

    def __iadd__(self, tys: List[Type]) -> AllDict:
        for ty in tys:
            key = ty.name.snake()
            assert key not in self
            self[key] = ty
        return self


TYPES = AllDict()

TYPES += [
    Struct(Name(["empty", "struct"]), []),
    Array(Int(32), 5),
    Struct(
        Name(["some", "struct"]), [
            Field(Name("a"), Int(8)),
            Field(Name("b"), Int(16)),
            Field(Name("c"), Array(Float(32), 2)),
        ]
    ),
]
TYPES += [
    Vector(Int(32)),
    Vector(Int(64)),
    String(),
    Vector(Array(Int(32), 2)),
    Vector(Array(Int(16, signed=True), 3)),
    Struct(Name(["value", "struct"]), [
        Field(Name("value"), Int(32)),
    ]),
    Struct(Name(["arrays", "struct"]), [
        Field(Name("idata"), Array(Int(32), 8)),
        Field(Name("fdata"), Array(Float(64), 4)),
    ]),
    Struct(Name(["int", "vector", "struct"]), [
        Field(Name("data"), Vector(Int(32))),
    ]),
    Struct(Name(["float", "vector", "struct"]), [
        Field(Name("data"), Vector(Float(32))),
    ]),
    Struct(Name(["string", "struct"]), [
        Field(Name("text"), String()),
    ]),
    Struct(
        Name(["integers", "struct"]), [
            Field(Name("u8_"), Int(8)),
            Field(Name("u16_"), Int(16)),
            Field(Name("u32_"), Int(32)),
            Field(Name("u64_"), Int(64)),
            Field(Name("i8_"), Int(8, signed=True)),
            Field(Name("i16_"), Int(16, signed=True)),
            Field(Name("i32_"), Int(32, signed=True)),
            Field(Name("i64_"), Int(64, signed=True)),
        ]
    ),
    Struct(Name(["floats"]), [
        Field(Name("f32_"), Float(32)),
        Field(Name("f64_"), Float(64)),
    ]),
    Struct(Name(["vector", "of", "arrays"]), [
        Field(Name("data"), Vector(TYPES["array5_uint32"])),
    ]),
    Struct(Name(["vector", "of", "structs"]), [
        Field(Name("data"), Vector(TYPES["some_struct"])),
    ]),
    Struct(
        Name(["nested", "struct"]), [
            Field(Name("u8_"), Int(8)),
            Field(
                Name("one"),
                Struct(Name(["nested", "struct", "one"]), [
                    Field(Name("u8_"), Int(8)),
                    Field(Name("u32_"), Int(32)),
                ])
            ),
            Field(
                Name("two"),
                Struct(
                    Name(["nested", "struct", "two"]), [
                        Field(Name("u8_"), Int(8)),
                        Field(Name("u32_"), Int(32)),
                        Field(Name("data"), Vector(Int(16))),
                    ]
                )
            ),
        ]
    ),
    Variant(
        Name(["sized", "variant"]),
        [
            Field(Name("empty"), TYPES["empty_struct"]),
            Field(Name("value"), Int(32)),
        ],
        sized=True,
    ),
    Variant(
        Name(["unsized", "variant"]),
        [
            Field(Name("empty"), TYPES["empty_struct"]),
            Field(Name("value"), Int(32)),
        ],
        sized=False,
    ),
    Variant(
        Name(["auto", "unsized", "variant"]),
        [
            Field(Name("empty"), TYPES["empty_struct"]),
            Field(Name("value"), Int(32)),
            Field(Name("vector"), Vector(Int(32))),
        ],
    ),
]
TYPES += [
    Struct(
        Name(["struct", "of", "unsized", "variant"]), [
            Field(Name("value"), Int(32)),
            Field(Name("data"), TYPES["auto_unsized_variant"]),
        ]
    ),
]
