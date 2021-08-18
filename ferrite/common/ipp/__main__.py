from . import *

app_msg = Variant(
    "IppAppMsg",
    [
        Field("start", Struct("_IppAppMsgStart")),
        Field("dac_wf", Struct("_IppAppMsgDacWf", [
            Field("data", Vector(Int(24))),
        ])),
    ],
)

print(app_msg.c_prelude().collect())
print(app_msg.cpp_prelude().collect())

mcu_msg = Variant(
    "IppMcuMsg",
    [
        Field("dac_wf_req", Struct("_IppMcuMsgDacWfReq")),
        Field("adc_wf", Struct("_IppMcuMsgAdcWf", [
            Field("index", Int(8)),
            Field("data", Vector(Int(24))),
        ])),
        Field("error", Struct("_IppMcuMsgError", [
            Field("code", Int(8)),
            Field("message", String()),
        ])),
        Field("error", Struct("_IppMcuMsgDebug", [
            Field("message", String()),
        ])),
    ],
)

print(mcu_msg.c_prelude().collect())
print(mcu_msg.cpp_prelude().collect())
