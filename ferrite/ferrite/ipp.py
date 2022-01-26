from pathlib import Path

from ferrite.codegen.base import Context, Name
from ferrite.codegen.all import Int, Array, Vector, String, Field
from ferrite.codegen.generate import make_variant, generate_and_write

AppMsg = make_variant(
    Name(["app", "msg"]),
    [
        (Name(["start"]), []),
        (Name(["stop"]), []),
        (Name(["dac", "set"]), [
            Field("value", Int(32, signed=True)),
        ]),
        (Name(["adc", "req"]), []),
        (Name(["dout", "set"]), [
            Field("value", Int(8, signed=False)),
        ]),
    ],
)

McuMsg = make_variant(
    Name(["mcu", "msg"]),
    [
        (Name(["adc", "val"]), [
            Field("values", Array(Int(32, signed=True), 6)),
        ]),
        (Name(["din", "val"]), [
            Field("value", Int(8, signed=False)),
        ]),
        (Name(["error"]), [
            Field("code", Int(8)),
            Field("message", String()),
        ]),
        (Name(["debug"]), [
            Field("message", String()),
        ]),
        (Name(["wf", "req"]), []),
        (Name(["adc", "wf"]), [
            Field("index", Int(8, signed=False)),
            Field("elements", Vector(Int(32, signed=True)))
        ]),
    ],
)


def generate(path: Path) -> None:
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
