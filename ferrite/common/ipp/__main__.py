from ipp.base import CONTEXT, Context, Name
from ipp.prim import Int
from ipp.container import Vector, String
from ipp.struct import Field
from ipp.codegen import make_variant, generate

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
        prefix="ipp",
        test_attempts=8,
    ),
)
