from __future__ import annotations
from typing import List, Optional, ClassVar

from random import Random
from dataclasses import dataclass
import string
import struct

import numpy as np
from numpy.typing import DTypeLike

from ferrite.codegen.base import CONTEXT, Location, Name, Type, Source
from ferrite.codegen.utils import is_power_of_2


@dataclass
class Int(Type):
    bits: int
    signed: bool = False

    _DTYPES: ClassVar[List[List[DTypeLike]]] = [
        [np.uint8, np.int8],
        [np.uint16, np.int16],
        [np.uint32, np.int32],
        [np.uint64, np.int64],
    ]

    def _is_builtin(self) -> bool:
        return is_power_of_2(self.bits // 8) and (self.bits % 8) == 0

    def __post_init__(self) -> None:
        assert self._is_builtin()
        super().__init__(sized=True, trivial=True)

    def name(self) -> Name:
        return Name(self._int_name())

    def size(self) -> int:
        return (self.bits - 1) // 8 + 1

    def load(self, data: bytes) -> int:
        assert len(data) == self.bits // 8
        return int.from_bytes(data, byteorder="little", signed=self.signed)

    def store(self, value: int) -> bytes:
        return value.to_bytes(self.bits // 8, byteorder="little", signed=self.signed)

    def default(self) -> int:
        return 0

    def random(self, rng: Random) -> int:
        if not self.signed:
            return rng.randrange(0, 2**self.bits)
        else:
            return rng.randrange(-2**(self.bits - 1), 2**(self.bits - 1))

    def is_instance(self, value: int) -> bool:
        return isinstance(value, int)

    def np_dtype(self) -> DTypeLike:
        return Int._DTYPES[(self.bits // 8).bit_length() - 1][self.signed]

    def _int_name(self) -> str:
        return f"{'u' if not self.signed else ''}int{self.bits}"

    def _int_literal(self, value: int, hex: bool = False) -> str:
        if hex:
            vstr = f"0x{value:x}"
        else:
            vstr = str(value)
        return f"{vstr}{'u' if not self.signed else ''}{'ll' if self.bits > 32 else ''}"

    def c_type(self) -> str:
        return self._int_name() + "_t"

    def rust_type(self) -> str:
        return ("i" if self.signed else "u") + str(self.bits)

    def pyi_type(self) -> str:
        return "int"

    def pyi_np_dtype(self) -> str:
        return f"np.{self._int_name()}"


@dataclass
class Float(Type):
    bits: int

    def __post_init__(self) -> None:
        if self.bits != 32 and self.bits != 64:
            raise RuntimeError(f"{self.bits}-bit float is not supported")
        super().__init__(sized=True, trivial=True)

    def name(self) -> Name:
        return Name(f"float{self.bits}")

    def size(self) -> int:
        return (self.bits - 1) // 8 + 1

    def load(self, data: bytes) -> float:
        assert len(data) == self.bits // 8
        value = struct.unpack(f"<{'f' if self.bits == 32 else 'd'}", data)[0]
        assert isinstance(value, float)
        return value

    def store(self, value: float) -> bytes:
        return struct.pack(f"<{'f' if self.bits == 32 else 'd'}", value)

    def default(self) -> float:
        return 0.0

    def random(self, rng: Random) -> float:
        return rng.gauss(0.0, 1.0)

    def is_instance(self, value: float) -> bool:
        return isinstance(value, float)

    def np_dtype(self) -> DTypeLike:
        return np.float32 if self.bits == 32 else np.float64

    def c_type(self) -> str:
        return "float" if self.bits == 32 else "double"

    def rust_type(self) -> str:
        return f"f{self.bits}"

    def pyi_type(self) -> str:
        return "float"

    def pyi_np_dtype(self) -> str:
        return f"np.float{self.bits}"


class Char(Type):

    def __init__(self) -> None:
        super().__init__(sized=True, trivial=True)

    def name(self) -> Name:
        return Name("char")

    def size(self) -> int:
        return 1

    def load(self, data: bytes) -> str:
        assert len(data) == 1
        return data.decode('ascii')

    def store(self, value: str) -> bytes:
        assert len(value) == 1
        return value.encode('ascii')

    def random(self, rng: Random) -> str:
        return rng.choice(string.ascii_letters + string.digits)

    def is_instance(self, value: str) -> bool:
        return isinstance(value, str) and len(value) == 1

    def c_type(self) -> str:
        return "char"

    def rust_type(self) -> str:
        return "u8"

    def pyi_type(self) -> str:
        return "str"


@dataclass
class Pointer(Type):
    type: Type
    const: bool = False
    _postfix: str = "ptr"

    def __post_init__(self) -> None:
        super().__init__(sized=True, trivial=True)

    def name(self) -> Name:
        return Name(self.type.name(), "const" if self.const else "", self._postfix)

    def c_type(self) -> str:
        return f"{'const ' if self.const else ''}{self.type.c_type()} *"

    def rust_type(self) -> str:
        return f"*{'const ' if self.const else 'mut '}{self.type.rust_type()}"

    def c_source(self) -> Optional[Source]:
        return self.type.c_source()

    def rust_source(self) -> Optional[Source]:
        return self.type.rust_source()


class Reference(Pointer):

    def __init__(self, type: Type, const: bool = False):
        super().__init__(type, const, _postfix="ref")

    def c_type(self) -> str:
        raise RuntimeError("C has no references")

    def rust_type(self) -> str:
        return f"&{'' if self.const else 'mut '}{self.type.rust_type()}"
