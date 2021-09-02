from typing import List, Tuple
from ipp.base import CONTEXT, Location, Name, Source, Type
from ipp.prim import Int
from ipp.container import Vector, String
from ipp.struct import Field, Struct
from ipp.variant import Variant

def make_variant(name: Name, messages: List[Tuple[Name, List[Field]]]) -> Variant:
    return Variant(
        name,
        [
            Field(suffux, Struct(Name(name, suffux), fields))
            for suffux, fields in messages
        ],
    )

CONTEXT.prefix = "ipp"

app_msg = make_variant(
    Name(["app", "msg"]),
    [
        (Name(["start"]), []),
        (Name(["dac", "wf"]), [
            Field("data", Vector(Int(24))),
        ]),
    ],
)

mcu_msg = make_variant(
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

with open("_out.h", "w") as f:
    source = Source(None, deps=[
        app_msg.c_source(),
        mcu_msg.c_source(),
    ])
    f.write("#pragma once\n\n")
    f.write("\n".join([
        "#include <stdlib.h>",
        "#include <stdint.h>",
        "#include <string.h>",
    ]) + "\n\n")
    f.write(source.make_source(Location.INCLUDES, separator="\n"))
    f.write("\n")
    f.write("\n".join([
        "#ifdef __cplusplus",
        "extern \"C\" {",
        "#endif // __cplusplus",
    ]) + "\n")
    f.write("\n")
    f.write(source.make_source(Location.DECLARATION))
    f.write("\n")
    f.write("\n".join([
        "#ifdef __cplusplus",
        "}",
        "#endif // __cplusplus",
    ]))
    f.write("\n")

with open("_out.hpp", "w") as f:
    source = Source(None, deps=[
        app_msg.cpp_source(),
        mcu_msg.cpp_source(),
    ])
    f.write("#pragma once\n\n")
    f.write(source.make_source(Location.INCLUDES, separator="\n"))
    f.write("\n")
    f.write("#include <_out.h>\n\n")
    f.write(f"namespace {CONTEXT.prefix} {{\n\n")
    f.write(source.make_source(Location.DECLARATION))
    f.write("\n")
    f.write(f"}} // namespace {CONTEXT.prefix}")
    f.write("\n")
