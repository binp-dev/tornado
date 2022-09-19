from __future__ import annotations

from pathlib import Path

from ferrite.protogen.base import Name
from ferrite.protogen.all import Int, Array, Vector, String, Field
from ferrite.protogen.generator import make_variant

AppMsg = make_variant(
    Name(["app", "msg"]),
    [
        (Name(["empty"]), []),
        (Name(["connect"]), []),
        (Name(["keep", "alive"]), []),
        (Name(["dout", "update"]), [
            Field(Name(["value"]), Int(8, signed=False)),
        ]),
        (Name(["dac", "mode"]), [
            Field(Name(["enable"]), Int(8, signed=False)),
        ]),
        (Name(["dac", "data"]), [
            Field(Name(["points"]), Vector(Int(32, signed=True))),
        ]),
        (Name(["stats", "reset"]), []),
    ],
)

McuMsg = make_variant(
    Name(["mcu", "msg"]),
    [
        (Name(["empty"]), []),
        (Name(["din", "update"]), [
            Field(Name(["value"]), Int(8, signed=False)),
        ]),
        (Name(["dac", "request"]), [
            Field(Name(["count"]), Int(32, signed=False)),
        ]),
        (Name(["adc", "data"]), [
            Field(Name(["points", "arrays"]), Vector(Array(Int(32, signed=True), 6))),
        ]),
        (Name(["error"]), [
            Field(Name(["code"]), Int(8, signed=False)),
            Field(Name(["message"]), String()),
        ]),
        (Name(["debug"]), [
            Field(Name(["message"]), String()),
        ]),
    ],
)
