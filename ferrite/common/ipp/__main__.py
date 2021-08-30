from typing import List
from ipp.base import Name, Source, Type
from ipp.prim import Int
from ipp.struct import Field, Struct, Variant
from ipp.container import Vector, String

def make_variant(name: Name, types: List[Type]) -> Variant:
    return Variant(
        name,
        [Field(ty.name().snake(), ty) for ty in types]
    )

app_msg = make_variant(
    Name(["App", "Msg"]),
    [
        Struct(Name(["app", "msg", "start"])),
        Struct(Name(["app", "msg", "dac", "wf"]), [
            Field("data", Vector(Int(24))),
        ]),
    ],
)

mcu_msg = make_variant(
    Name(["Mcu", "Msg"]),
    [
        Struct(Name(["mcu", "msg", "dac", "wf", "req"])),
        Struct(Name(["mcu", "msg", "adc", "wf"]), [
            Field("index", Int(8)),
            Field("data", Vector(Int(24))),
        ]),
        Struct(Name(["mcu", "msg", "error"]), [
            Field("code", Int(8)),
            Field("message", String()),
        ]),
        Struct(Name(["mcu", "msg", "debug"]), [
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
