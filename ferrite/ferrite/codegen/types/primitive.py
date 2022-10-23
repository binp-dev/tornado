from __future__ import annotations
from typing import Any, ClassVar, List, TypeVar, Optional

from random import Random
import string
import struct
from enum import IntEnum

import numpy as np
from numpy.typing import DTypeLike

from ferrite.codegen.base import Location, Name, Source, UnexpectedEof
from ferrite.codegen.utils import is_power_of_2

from .base import Type

T = TypeVar('T')


class Int(Type):

    class Bits(IntEnum):
        BYTE = 8,
        SIZE = struct.calcsize("P") * 8,

    _np_dtypes: ClassVar[List[List[DTypeLike]]] = [
        [np.uint8, np.int8],
        [np.uint16, np.int16],
        [np.uint32, np.int32],
        [np.uint64, np.int64],
    ]

    def __init__(self, bits: int | Bits, signed: bool = False, portable: Optional[bool] = None) -> None:
        assert is_power_of_2(bits // 8) and bits % 8 == 0
        self._int_name = f"{'u' if not signed else ''}int{bits}"
        super().__init__(Name(self._int_name), bits // 8)
        self.bits = bits
        self.signed = signed

        if portable is not None:
            self.portable = portable
            assert not isinstance(bits, Int.Bits)
        else:
            self.portable = not isinstance(bits, Int.Bits)

    def load(self, data: bytes) -> int:
        assert self.portable
        if len(data) < self.size:
            raise UnexpectedEof(self, data)
        return int.from_bytes(data[:self.size], byteorder="little", signed=self.signed)

    def store(self, value: int) -> bytes:
        assert self.portable
        return value.to_bytes(self.size, byteorder="little", signed=self.signed)

    def default(self) -> int:
        return 0

    def random(self, rng: Random) -> int:
        if not self.signed:
            return rng.randrange(0, 2**self.bits)
        else:
            return rng.randrange(-2**(self.bits - 1), 2**(self.bits - 1))

    def is_instance(self, value: Any) -> bool:
        return isinstance(value, int)

    def is_np(self) -> bool:
        return True

    def np_dtype(self) -> DTypeLike:
        return self._np_dtypes[(self.bits // 8).bit_length() - 1][self.signed]

    def np_shape(self) -> List[int]:
        return []

    def c_type(self) -> str:
        if self.bits != Int.Bits.SIZE:
            return self._int_name + "_t"
        else:
            return ("s" if self.signed else "") + "size_t"

    def rust_type(self) -> str:
        if self.portable and self.bits > 8:
            return ("I" if self.signed else "U") + str(self.bits)
        else:
            if self.bits != Int.Bits.SIZE:
                return ("i" if self.signed else "u") + str(self.bits)
            else:
                return ("i" if self.signed else "u") + "size"

    def pyi_type(self) -> str:
        return "int"

    def _pyi_np_dtype(self) -> str:
        return f"np.{self._int_name}"

    def c_check(self, var: str, obj: int) -> List[str]:
        return [f"codegen_assert_eq({var}, {self.c_object(obj)});"]

    def c_object(self, obj: int, hex: bool = False) -> str:
        if hex:
            vstr = f"0x{obj:x}"
        else:
            vstr = str(obj)
        return f"{vstr}{'u' if not self.signed else ''}{'ll' if self.bits > 32 else ''}"

    def rust_check(self, var: str, obj: int) -> List[str]:
        if self.portable and self.bits > 8:
            var = f"{var}.to_native()"
        else:
            var = f"*{var}"
        return [f"assert_eq!({var}, {obj});"]

    def rust_object(self, obj: int) -> str:
        return str(obj)

    def rust_source(self) -> Optional[Source]:
        if self.portable and self.bits > 8:
            return None
        else:
            return Source(Location.IMPORT, [["use flatty::portable::le::*;"]])


class Float(Type):

    def _choose(self, f32: T, f64: T) -> T:
        if self.bits == 32:
            return f32
        elif self.bits == 64:
            return f64
        else:
            raise RuntimeError("Float type is neither 'f32' nor 'f64'")

    def __init__(self, bits: int, portable: bool = True) -> None:
        if bits != 32 and bits != 64:
            raise RuntimeError(f"{bits}-bit float is not supported")
        super().__init__(Name(f"float{bits}"), bits // 8)
        self.bits = bits
        self.portable = portable

    def load(self, data: bytes) -> float:
        if len(data) < self.size:
            raise UnexpectedEof(self, data)
        value = struct.unpack(self._choose("<f", "<d"), data[:self.size])[0]
        assert isinstance(value, float)
        return value

    def store(self, value: float) -> bytes:
        return struct.pack(self._choose("<f", "<d"), value)

    def default(self) -> float:
        return 0.0

    def random(self, rng: Random) -> float:
        return rng.gauss(0.0, 1.0)

    def is_instance(self, value: Any) -> bool:
        return isinstance(value, float)

    def is_np(self) -> bool:
        return True

    def np_dtype(self) -> DTypeLike:
        return self._choose(np.float32, np.float64)

    def np_shape(self) -> List[int]:
        return []

    def c_type(self) -> str:
        return self._choose("float", "double")

    def rust_type(self) -> str:
        if self.portable:
            return f"F{self.bits}"
        else:
            return f"f{self.bits}"

    def pyi_type(self) -> str:
        return "float"

    def _pyi_np_dtype(self) -> str:
        return f"np.float{self.bits}"

    def c_check(self, var: str, obj: float) -> List[str]:
        return [f"codegen_assert_eq({var}, {self.c_object(obj)});"]

    def c_object(self, obj: float) -> str:
        return f"{obj}{self._choose('f', '')}"

    def rust_check(self, var: str, obj: float) -> List[str]:
        if self.portable:
            var = f"{var}.to_native()"
        else:
            var = f"*{var}"
        return [f"assert_eq!({var}, {self.rust_object(obj)});"]

    def rust_object(self, obj: float) -> str:
        return str(obj)

    def rust_source(self) -> Optional[Source]:
        if self.portable:
            return Source(Location.IMPORT, [["use flatty::portable::le::*;"]])
        else:
            return None


class Char(Type):

    def __init__(self) -> None:
        super().__init__(Name("char"), 1)

    def load(self, data: bytes) -> str:
        if len(data) < 1:
            raise UnexpectedEof(self, data)
        return data.decode('ascii')

    def store(self, value: str) -> bytes:
        assert len(value) == 1
        return value.encode('ascii')

    def random(self, rng: Random) -> str:
        return rng.choice(string.ascii_letters + string.digits)

    def is_instance(self, value: Any) -> bool:
        return isinstance(value, str) and len(value) == 1

    def c_type(self) -> str:
        return "char"

    def rust_type(self) -> str:
        return "u8"

    def pyi_type(self) -> str:
        return "str"

    def _code(self, obj: str) -> int:
        return obj.encode('ascii')[0]

    def c_check(self, var: str, obj: str) -> List[str]:
        return [f"codegen_assert_eq({var}, {self.c_object(obj)});"]

    def c_object(self, obj: str) -> str:
        return f"'\\x{self._code(obj):02x}'"

    def rust_check(self, var: str, obj: str) -> List[str]:
        return [f"assert_eq!(*{var}, {self.rust_object(obj)});"]

    def rust_object(self, obj: str) -> str:
        return str(self._code(obj))
