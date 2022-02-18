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

all_: List[Type[Any]] = [
    Array(Int(24), 5),
    Vector(Int(24)),
    Vector(Int(64)),
    String(),
    empty,
    Struct(Name(["value", "struct"]), [
        Field("value", Int(32)),
    ]),
    Struct(Name(["vector", "struct"]), [
        Field("data", Vector(Int(32))),
    ]),
    Struct(Name(["string", "struct"]), [
        Field("text", String()),
    ]),
    Struct(
        Name(["integers"]), [
            Field("u8", Int(8)),
            Field("u16", Int(16)),
            Field("u24", Int(24)),
            Field("u32", Int(32)),
            Field("u48", Int(48)),
            Field("u56", Int(56)),
            Field("u64", Int(64)),
            Field("i8", Int(8, signed=True)),
            Field("i16", Int(16, signed=True)),
            Field("i24", Int(24, signed=True)),
            Field("i32", Int(32, signed=True)),
            Field("i64", Int(64, signed=True)),
        ]
    ),
    Struct(Name(["floats"]), [
        Field("f32", Float(32)),
        Field("f64", Float(64)),
    ]),
    Struct(Name(["non", "trivial", "vector", "struct"]), [
        Field("data", Vector(Int(24, signed=True))),
    ]),
    Struct(
        Name(["nested", "struct"]), [
            Field("u8", Int(8)),
            Field("one", Struct(Name(["nested", "struct", "one"]), [
                Field("u8", Int(8)),
                Field("u24", Int(32)),
            ])),
            Field(
                "two",
                Struct(
                    Name(["nested", "struct", "two"]), [
                        Field("u8", Int(8)),
                        Field("u24", Int(32)),
                        Field("data", Vector(Int(24))),
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
        Context(
            prefix="codegen",
            test_attempts=16,
        ),
    )
