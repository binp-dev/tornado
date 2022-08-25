from __future__ import annotations
from typing import Any, List, TypeVar

from random import Random
import string
import struct

from ferrite.codegen.base import Name, Type
from ferrite.codegen.utils import is_power_of_2

T = TypeVar('T')


class Int(Type):
    bits: int
    signed: bool = False

    def __init__(self, bits: int, signed: bool = False) -> None:
        assert is_power_of_2(bits // 8) and bits % 8 == 0
        self._int_name = f"{'u' if not signed else ''}int{bits}"
        super().__init__(Name(self._int_name), bits // 8, bits // 8)
        self.bits = bits
        self.signed = signed

    def load(self, data: bytes) -> int:
        assert len(data) >= self.size
        return int.from_bytes(data, byteorder="little", signed=self.signed)

    def store(self, value: int) -> bytes:
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

    def _c_literal(self, value: int, hex: bool = False) -> str:
        if hex:
            vstr = f"0x{value:x}"
        else:
            vstr = str(value)
        return f"{vstr}{'u' if not self.signed else ''}{'ll' if self.bits > 32 else ''}"

    def c_type(self) -> str:
        return self._int_name + "_t"

    def rust_type(self) -> str:
        return ("i" if self.signed else "u") + str(self.bits)

    def pyi_type(self) -> str:
        return "int"

    def c_check(self, var: str, obj: int) -> List[str]:
        return [f"codegen_assert_eq({var}, {self._c_literal(obj)});"]

    def rust_check(self, var: str, obj: int) -> List[str]:
        return [f"assert_eq!({var}, {obj});"]

    def rust_object(self, obj: int) -> str:
        return str(obj)


class Float(Type):

    def _choose(self, f32: T, f64: T) -> T:
        if self.bits == 32:
            return f32
        elif self.bits == 64:
            return f64
        else:
            raise RuntimeError("Float type is neither 'f32' nor 'f64'")

    def __init__(self, bits: int) -> None:
        if bits != 32 and bits != 64:
            raise RuntimeError(f"{bits}-bit float is not supported")
        super().__init__(Name(f"float{bits}"), bits // 8, bits // 8)
        self.bits = bits

    def load(self, data: bytes) -> float:
        assert len(data) >= self.size
        value = struct.unpack(self._choose("<f", "<d"), data)[0]
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

    def _c_literal(self, value: float) -> str:
        return f"{value}{self._choose('f', '')}"

    def c_type(self) -> str:
        return self._choose("float", "double")

    def rust_type(self) -> str:
        return f"f{self.bits}"

    def pyi_type(self) -> str:
        return "float"

    def c_check(self, var: str, obj: float) -> List[str]:
        return [f"codegen_assert_eq({var}, {self._c_literal(obj)});"]

    def rust_check(self, var: str, obj: float) -> List[str]:
        return [f"assert_eq!({var}, {obj});"]

    def rust_object(self, obj: float) -> str:
        return str(obj)


class Char(Type):

    def __init__(self) -> None:
        super().__init__(Name("char"), 1, 1)

    def load(self, data: bytes) -> str:
        assert len(data) >= 1
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
        return [f"codegen_assert_eq({var}, '\\x{self._code(obj):02x}');"]

    def rust_check(self, var: str, obj: str) -> List[str]:
        return [f"assert_eq!({var}, {self._code(obj)});"]

    def rust_object(self, obj: str) -> str:
        return str(self._code(obj))
