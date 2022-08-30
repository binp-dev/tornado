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
    Field(Name("value"), Vector(Int(32, signed=True))),
])
Aao = Struct(Name("aao"), [
    Field(Name("value"), Vector(Int(32, signed=True))),
])

InMsg = Variant(Name("in", "msg"), [
    Field(Ai.name, Ai),
    Field(Aai.name, Aai),
])
OutMsg = Variant(Name("out", "msg"), [
    Field(Ao.name, Ao),
    Field(Aao.name, Aao),
])
