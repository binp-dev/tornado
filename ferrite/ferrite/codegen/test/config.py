from __future__ import annotations

from ferrite.codegen.types import Int

Typedef = Int(32, signed=True)

SOME_NUM: Int(32, signed=True, portable=False) = 10 # type: ignore
SOME_SIZE: Int(Int.Bits.SIZE) = 10 # type: ignore
