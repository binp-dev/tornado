from __future__ import annotations

from pathlib import Path

from ferrite.codegen.base import Context, Name
from ferrite.codegen.all import Int, Vector, String, Field
from ferrite.codegen.generate import make_variant, generate_and_write

AppMsg = make_variant(
    Name(["app", "msg"]),
    [
        (Name(["connect"]), []),
        (Name(["start", "dac"]), []),
        (Name(["stop", "dac"]), []),
        (Name(["keep", "alive"]), []),
        (Name(["dout", "set"]), [
            Field("value", Int(8, signed=False)),
        ]),
        (Name(["dac", "wf"]), [
            Field("elements", Vector(Int(32, signed=True))),
        ]),
    ],
)

McuMsg = make_variant(
    Name(["mcu", "msg"]),
    [
        (Name(["din", "val"]), [
            Field("value", Int(8, signed=False)),
        ]),
        (Name(["dac", "wf", "req"]), [
            Field("count", Int(32, signed=False)),
        ]),
        (Name(["adc", "wf"]), [
            Field("index", Int(8, signed=False)),
            Field("elements", Vector(Int(32, signed=True))),
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
