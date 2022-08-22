from __future__ import annotations
from typing import Any, List

from pathlib import Path

from ferrite.codegen.variant import Variant
from ferrite.codegen.base import Context, Name, Type
from ferrite.codegen.primitive import Float, Int
from ferrite.codegen.container import Array, Vector, String
from ferrite.codegen.structure import Field, Struct
from ferrite.codegen.generate import generate_and_write

empty = Struct(Name(["empty", "struct"]), [])
array = Array(Int(32), 5)
struct = Struct(Name(["some", "struct"]), [
    Field("a", Int(8)),
    Field("b", Int(16)),
    Field("c", Array(Float(32), 2)),
])

all_: List[Type] = [
    Vector(Int(32)),
    Vector(Int(64)),
    String(),
    Vector(Array(Int(32), 2)),
    Vector(Array(Int(16, signed=True), 3)),
    Struct(Name(["value", "struct"]), [
        Field("value", Int(32)),
    ]),
    Struct(Name(["arrays", "struct"]), [
        Field("idata", Array(Int(32), 8)),
        Field("fdata", Array(Float(64), 4)),
    ]),
    Struct(Name(["int", "vector", "struct"]), [
        Field("data", Vector(Int(32))),
    ]),
    Struct(Name(["float", "vector", "struct"]), [
        Field("data", Vector(Float(32))),
    ]),
    Struct(Name(["string", "struct"]), [
        Field("text", String()),
    ]),
    Struct(
        Name(["integers", "struct"]), [
            Field("u8_", Int(8)),
            Field("u16_", Int(16)),
            Field("u32_", Int(32)),
            Field("u64_", Int(64)),
            Field("i8_", Int(8, signed=True)),
            Field("i16_", Int(16, signed=True)),
            Field("i32_", Int(32, signed=True)),
            Field("i64_", Int(64, signed=True)),
        ]
    ),
    Struct(Name(["floats"]), [
        Field("f32_", Float(32)),
        Field("f64_", Float(64)),
    ]),
    Struct(Name(["vector", "of", "arrays"]), [
        Field("data", Vector(array)),
    ]),
    Struct(Name(["vector", "of", "structs"]), [
        Field("data", Vector(struct)),
    ]),
    Struct(
        Name(["nested", "struct"]), [
            Field("u8_", Int(8)),
            Field("one", Struct(Name(["nested", "struct", "one"]), [
                Field("u8_", Int(8)),
                Field("u32_", Int(32)),
            ])),
            Field(
                "two",
                Struct(
                    Name(["nested", "struct", "two"]), [
                        Field("u8_", Int(8)),
                        Field("u32_", Int(32)),
                        Field("data", Vector(Int(16))),
                    ]
                )
            ),
        ]
    ),
    Variant(
        Name(["sized", "variant"]),
        [
            Field("empty", empty),
            Field("value", Int(32)),
        ],
        sized=True,
    ),
    Variant(
        Name(["unsized", "variant"]),
        [
            Field("empty", empty),
            Field("value", Int(32)),
        ],
        sized=False,
    ),
    Variant(
        Name(["auto", "unsized", "variant"]),
        [
            Field("empty", empty),
            Field("value", Int(32)),
            Field("vector", Vector(Int(32))),
        ],
    ),
]


def generate(path: Path) -> None:
    generate_and_write(
        all_,
        path,
        Context(prefix="codegen",),
    )
