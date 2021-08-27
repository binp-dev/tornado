from ipp.base import Source
from ipp.prim import Int
from ipp.struct import Field, Struct, Variant
from ipp.container import Vector, String

app_msg = Variant(
    "IppAppMsg",
    [
        Field("start", Struct("_IppAppMsgStart")),
        Field("dac_wf", Struct("_IppAppMsgDacWf", [
            Field("data", Vector(Int(32))),
        ])),
    ],
)

mcu_msg = Variant(
    "IppMcuMsg",
    [
        Field("dac_wf_req", Struct("_IppMcuMsgDacWfReq")),
        Field("adc_wf", Struct("_IppMcuMsgAdcWf", [
            Field("index", Int(8)),
            Field("data", Vector(Int(32))),
        ])),
        Field("error", Struct("_IppMcuMsgError", [
            Field("code", Int(8)),
            Field("message", String()),
        ])),
        Field("debug", Struct("_IppMcuMsgDebug", [
            Field("message", String()),
        ])),
    ],
)

with open("_out.h", "w") as f:
    f.write("#pragma once\n\n")
    f.write("#include <stdint.h>\n\n")
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
