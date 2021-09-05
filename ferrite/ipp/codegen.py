import os
from codegen.base import CONTEXT, Context, Include, Name
from codegen.prim import Int
from codegen.container import Vector, String
from codegen.struct import Field
from codegen.text import make_variant, generate_and_write


AppMsg = make_variant(
    Name(["app", "msg"]),
    [
        (Name(["start"]), []),
        (Name(["dac", "wf"]), [
            Field("data", Vector(Int(24))),
        ]),
    ],
)

McuMsg = make_variant(
    Name(["mcu", "msg"]),
    [
        (Name(["dac", "wf", "req"]), []),
        (Name(["adc", "wf"]), [
            Field("index", Int(8)),
            Field("data", Vector(Int(24))),
        ]),
        (Name(["error"]), [
            Field("code", Int(8)),
            Field("message", String()),
        ]),
        (Name(["debug"]), [
            Field("message", String()),
        ]),
    ],
)

def generate(path: str):
    generate_and_write(
        [
            AppMsg,
            McuMsg,
        ],
        path,
        Context(
            prefix="ipp",
            test_attempts=8,
        ),
    )
