from codegen.base import Context, Name
from codegen.prim import Int
from codegen.container import String, Vector
from codegen.struct import Field
from codegen.text import make_variant, generate_and_write


AppMsg = make_variant(
    Name(["app", "msg"]),
    [
        (Name(["start"]), []),
        (Name(["stop"]), []),
        (Name(["dac", "set"]), [
            Field("value", Int(24)),
        ]),
        (Name(["adc", "req"]), [
            Field("index", Int(8)),
        ]),
        (Name(["wf", "data"]), [
            Field("data", Vector(Int(32, signed=True))),
        ]),
    ],
)

McuMsg = make_variant(
    Name(["mcu", "msg"]),
    [
        (Name(["adc", "val"]), [
            Field("index", Int(8)),
            Field("value", Int(24)),
        ]),
        (Name(["error"]), [
            Field("code", Int(8)),
            Field("message", String()),
        ]),
        (Name(["debug"]), [
            Field("message", String()),
        ]),
        (Name(["wf", "req"]), []),
        (Name(["wf", "data"]), [
            Field("data", Vector(Int(32, signed=True))),
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
