from __future__ import annotations
from typing import List

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

Imsg = Variant(Name("imsg"), [
    Field(Ai.name, Ai),
    Field(Aai.name, Aai),
])
Omsg = Variant(Name("omsg"), [
    Field(Ao.name, Ao),
    Field(Aao.name, Aao),
])
