from __future__ import annotations

from pathlib import Path

from ferrite.codegen.base import Context, Name
from ferrite.codegen.all import Int, Array, Vector, String, Field
from ferrite.codegen.generate import make_variant, generate_and_write

AppMsg = make_variant(
    Name(["app", "msg"]),
    [
        (Name(["connect"]), []),
        (Name(["keep", "alive"]), []),
        (Name(["dout", "update"]), [
            Field("value", Int(8, signed=False)),
        ]),
        (Name(["dac", "mode"]), [
            Field("enable", Int(8, signed=False)),
        ]),
        (Name(["dac", "data"]), [
            Field("points", Vector(Int(32, signed=True))),
        ]),
        (Name(["stats", "reset"]), []),
    ],
)

McuMsg = make_variant(
    Name(["mcu", "msg"]),
    [
        (Name(["din", "update"]), [
            Field("value", Int(8, signed=False)),
        ]),
        (Name(["dac", "request"]), [
            Field("count", Int(32, signed=False)),
        ]),
        (Name(["adc", "data"]), [
            Field("points_arrays", Vector(Array(Int(32, signed=True), 6))),
        ]),
        (Name(["error"]), [
            Field("code", Int(8, signed=False)),
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
