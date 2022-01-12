from pathlib import Path

from ferrite.codegen.base import Context, Name
from ferrite.codegen.all import Int, Array, String, Field
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
    ],
)

McuMsg = make_variant(
    Name(["mcu", "msg"]),
    [
        (Name(["adc", "val"]), [
            Field("values", Array(Int(32, signed=True), 6)),
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
