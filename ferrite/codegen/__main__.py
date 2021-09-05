from codegen.base import CONTEXT, Context, Name
from codegen.prim import Int
from codegen.container import Vector, String
from codegen.struct import Field
from codegen.codegen import make_variant, generate

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

generate(
    [
        AppMsg,
        McuMsg,
    ],
    "",
    Context(
        prefix="codegen",
        test_attempts=8,
    ),
)
