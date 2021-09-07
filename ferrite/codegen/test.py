import os
from codegen.base import Context, Name
from codegen.prim import Float, Int
from codegen.container import Vector, String
from codegen.struct import Field, Struct
from codegen.text import make_variant, generate_and_write


Msg = make_variant(
    Name(["msg"]),
    [
        (Name(["empty"]), []),
        (Name(["value"]), [
            Field("value", Int(24)),
        ]),
        (Name(["vector"]), [
            Field("data", Vector(Int(24))),
        ]),
        (Name(["string"]), [
            Field("text", String()),
        ]),
        (Name(["integers"]), [
            Field("u8", Int(8)),
            Field("u16", Int(16)),
            Field("u24", Int(24)),
            Field("u32", Int(32)),
            Field("u48", Int(48)),
            Field("u56", Int(56)),
            Field("u64", Int(64)),
            Field("i8", Int(8, signed=True)),
            Field("i16", Int(16, signed=True)),
            Field("i32", Int(32, signed=True)),
            Field("i64", Int(64, signed=True)),
        ]),
        (Name(["floats"]), [
            Field("f32", Float(32)),
            Field("f64", Float(64)),
        ]),
        (Name(["nested"]), [
            Field("u8", Int(8)),
            Field("one", Struct(Name(["msg", "nested", "one"]), [
                Field("u8", Int(8)),
                Field("u24", Int(32)),
            ])),
            Field("two", Struct(Name(["msg", "nested", "two"]), [
                Field("u8", Int(8)),
                Field("u24", Int(32)),
                Field("data", Vector(Int(24))),
            ])),
        ]),
    ],
)

def generate(path: str):
    generate_and_write(
        [Msg],
        path,
        Context(
            prefix="codegen",
            test_attempts=16,
        ),
    )
