from __future__ import annotations

from ferrite.codegen.base import Name
from ferrite.codegen.all import Int, Vector, Field, Struct, Variant

Ai = Struct(Name("ai"), [
    Field(Name("value"), Int(32, signed=True)),
])
Ao = Struct(Name("ao"), [
    Field(Name("value"), Int(32, signed=True)),
])
Aai = Struct(Name("aai"), [
    Field(Name("values"), Vector(Int(32, signed=True))),
])
Aao = Struct(Name("aao"), [
    Field(Name("values"), Vector(Int(32, signed=True))),
])
Waveform = Struct(Name("waveform"), [
    Field(Name("values"), Vector(Int(32, signed=True))),
])
Bi = Struct(Name("bi"), [
    Field(Name("value"), Int(32, signed=False)),
])
Bo = Struct(Name("bo"), [
    Field(Name("value"), Int(32, signed=False)),
])
MbbiDirect = Struct(Name("mbbi", "direct"), [
    Field(Name("value"), Int(32, signed=False)),
])
MbboDirect = Struct(Name("mbbo", "direct"), [
    Field(Name("value"), Int(32, signed=False)),
])

InMsg = Variant(
    Name("in", "msg"),
    [
        Field(Ai.name, Ai),
        Field(Aai.name, Aai),
        Field(Waveform.name, Waveform),
        Field(Bi.name, Bi),
        Field(MbbiDirect.name, MbbiDirect),
    ],
)
OutMsg = Variant(
    Name("out", "msg"),
    [
        Field(Ao.name, Ao),
        Field(Aao.name, Aao),
        Field(Bo.name, Bo),
        Field(MbboDirect.name, MbboDirect),
    ],
)
