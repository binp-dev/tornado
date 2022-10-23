from __future__ import annotations

from ferrite.codegen.types import Int

NativeType = Int(32, signed=True)
PortableType = Int(32, signed=True, portable=True)

SOME_NUM: Int(32, signed=True, portable=True) = 10 # type: ignore
SOME_SIZE: Int(Int.SIZE) = 10 # type: ignore
