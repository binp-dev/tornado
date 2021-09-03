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

c_source = Source(None, deps=[
    app_msg.c_source(),
    mcu_msg.c_source(),
])
cpp_source = Source(None, deps=[
    app_msg.cpp_source(),
    mcu_msg.cpp_source(),
])

with open("ipp.h", "w") as f:
    f.write("#pragma once\n\n")
    f.write("\n".join([
        "#include <stdlib.h>",
        "#include <stdint.h>",
        "#include <string.h>",
    ]) + "\n\n")
    f.write(c_source.make_source(Location.INCLUDES, separator="\n"))
    f.write("\n")
    f.write("\n".join([
        "#ifdef __cplusplus",
        "extern \"C\" {",
        "#endif // __cplusplus",
    ]) + "\n")
    f.write("\n")
    f.write(c_source.make_source(Location.DECLARATION))
    f.write("\n")
    f.write("\n".join([
        "#ifdef __cplusplus",
        "}",
        "#endif // __cplusplus",
    ]))
    f.write("\n")

with open("ipp.c", "w") as f:
    f.write("#include <ipp.h>\n\n")
    f.write(c_source.make_source(Location.DEFINITION))
    f.write("\n")

with open("ipp.hpp", "w") as f:
    f.write("#pragma once\n\n")
    f.write(cpp_source.make_source(Location.INCLUDES, separator="\n"))
    f.write("\n")
    f.write("#include <ipp.h>\n\n")
    f.write(f"namespace {CONTEXT.prefix} {{\n\n")
    f.write(cpp_source.make_source(Location.DECLARATION))
    f.write("\n\n")
    f.write(f"}} // namespace {CONTEXT.prefix}")
    f.write("\n")

with open("ipp.cpp", "w") as f:
    f.write("#include <ipp.hpp>\n\n")
    f.write(f"namespace {CONTEXT.prefix} {{\n\n")
    f.write(cpp_source.make_source(Location.DEFINITION))
    f.write("\n\n")
    f.write(f"}} // namespace {CONTEXT.prefix}")
    f.write("\n")

with open("ipp_test.cpp", "w") as f:
    f.write("#include <ipp.hpp>\n\n")
    f.write("#include <gtest/gtest.h>\n\n")
    f.write(f"using namespace {CONTEXT.prefix};\n\n")
    f.write(cpp_source.make_source(Location.TESTS))
    f.write("\n\n")
    f.write("\n".join([
        "int main(int argc, char **argv) {",
        "    testing::InitGoogleTest(&argc, argv);",
        "    return RUN_ALL_TESTS();",
        "}",
    ]))
    f.write("\n")
