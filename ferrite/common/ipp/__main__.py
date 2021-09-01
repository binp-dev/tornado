from typing import List, Tuple
from ipp.base import Name, Source, Type
from ipp.prim import Int
from ipp.struct import Field, Struct, Variant
from ipp.container import Vector, String

def make_variant(name: Name, messages: List[Tuple[Name, List[Field]]]) -> Variant:
    return Variant(
        name,
        [
            Field(suffux, Struct(Name(name, suffux), fields))
            for suffux, fields in messages
        ],
    )

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
    f.write("#pragma once\n\n")
    f.write("#include <stdlib.h>\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write("#include <string.h>\n\n")
    f.write("\n".join([
        "#ifdef __cplusplus",
        "extern \"C\" {",
        "#endif // __cplusplus",
    ]) + "\n")
    f.write("\n")
    f.write(Source(deps=[
        app_msg.c_source(),
        mcu_msg.c_source(),
    ]).make_source())
    f.write("\n")
    f.write("\n".join([
        "#ifdef __cplusplus",
        "};",
        "#endif // __cplusplus",
    ]) + "\n")

with open("_out.hpp", "w") as f:
    f.write("#pragma once\n\n")
    f.write("#include <_out.h>\n\n")
    f.write(Source(deps=[
        app_msg.cpp_source(),
        mcu_msg.cpp_source(),
    ]).make_source())
