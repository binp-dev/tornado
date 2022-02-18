from __future__ import annotations
from typing import Any, Callable, List, Optional

from random import Random
from dataclasses import dataclass
import string
import struct

from ferrite.codegen.base import CONTEXT, Location, Name, Type, Source
from ferrite.codegen.macros import ErrorKind, io_error, monostate, stream_read, stream_write, try_unwrap
from ferrite.codegen.utils import ceil_to_power_of_2, indent, is_power_of_2


@dataclass
class Int(Type[int]):
    bits: int
    signed: bool = False

    def _is_builtin(self) -> bool:
        return is_power_of_2(self.bits // 8) and (self.bits % 8) == 0

    def __post_init__(self) -> None:
        super().__init__(sized=True, trivial=self._is_builtin())

    def name(self) -> Name:
        return Name(("u" if not self.signed else "") + "int" + str(self.bits))

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

    @staticmethod
    def _int_name(bits: int, signed: bool = False) -> str:
        return f"{'u' if not signed else ''}int{bits}"

    @staticmethod
    def _int_type(bits: int, signed: bool = False) -> str:
        return Int._int_name(bits, signed) + "_t"

    @staticmethod
    def _int_literal(value: int, bits: int, signed: bool = False, hex: bool = False) -> str:
        if hex:
            vstr = f"0x{value:x}"
        else:
            vstr = str(value)
        return f"{vstr}{'u' if not signed else ''}{'ll' if bits > 32 else ''}"

    def c_type(self) -> str:
        ident = self._int_type(self.bits, self.signed)
        if not self.trivial and CONTEXT.prefix is not None:
            ident = CONTEXT.prefix + "_" + ident
        return ident

    def _ceil_type(self) -> str:
        return self._int_type(ceil_to_power_of_2(self.bits), self.signed)

    def _extend_sign(self, value: str) -> List[str]:
        if is_power_of_2(self.bits):
            return []
        ceil_bits = ceil_to_power_of_2(self.bits)
        mask = ((1 << (ceil_bits - self.bits)) - 1) << self.bits
        literal = Int._int_literal(mask, self.bits, False, hex=True)
        return [
            f"if (((({self._int_type(ceil_bits, False)}){value} >> {self.bits - 1}) & 1) != 0) {{",
            f"    {value} |= ({self.cpp_type()}){literal};",
            f"}}",
        ]

    def _check_bounds(self, value: str) -> List[str]:
        if is_power_of_2(self.bits):
            return []
        ceil_bits = ceil_to_power_of_2(self.bits)
        mask = (1 << (ceil_bits - self.bits + 1)) - 1
        zero = Int._int_literal(0, self.bits, False)
        literal = Int._int_literal(mask, self.bits, False, hex=True)
        err_cond = [
            f"({value} & {literal}) != 0",
            f"auto tmp = ({self._int_type(ceil_bits, False)}){value} >> {self.bits - 1}; tmp != {literal} && tmp != {zero}",
        ][self.signed]
        return [
            f"if ({err_cond}) {{",
            f"    return Err({io_error(ErrorKind.INVALID_DATA)});",
            f"}}",
        ]

    def c_source(self) -> Optional[Source]:
        if self.bits % 8 != 0 or self.bits > 64:
            raise RuntimeError(f"{self.bits}-bit integer is not supported")
        bytes = self.size()

        if self.trivial:
            return None

        prefix = f"{CONTEXT.prefix}_" if CONTEXT.prefix is not None else ""
        int_pref = self._int_name(self.bits, self.signed)
        load_decl = f"{self._ceil_type()} {prefix}{int_pref}_load({self.c_type()} x)"
        store_decl = f"{self.c_type()} {prefix}{int_pref}_store({self._ceil_type()} y)"
        declaraion = Source(
            Location.DECLARATION,
            [
                [
                    f"typedef struct {self.c_type()} {{",
                    f"    uint8_t bytes[{bytes}];",
                    f"}} {self.c_type()};",
                ],
                [f"{load_decl};"],
                [f"{store_decl};"],
            ],
        )
        return Source(
            Location.DEFINITION,
            [
                [
                    f"{load_decl} {{",
                    f"    {self._ceil_type()} y = 0;",
                    f"    memcpy((void *)&y, (const void *)&x, {self.size()});",
                    *indent(self._extend_sign("y") if self.signed else []),
                    f"    return y;",
                    f"}}",
                ],
                [
                    f"{store_decl} {{",
                    f"    {self.c_type()} x;",
                    f"    memcpy((void *)&x, (const void *)&y, {self.size()});",
                    f"    return x;",
                    f"}}",
                ],
            ],
            deps=[declaraion],
        )

    def cpp_type(self) -> str:
        return self._ceil_type()

    def cpp_source(self) -> Optional[Source]:
        if self.trivial:
            return super().cpp_source()

        load_decl = self._cpp_load_func_decl("stream")
        store_decl = self._cpp_store_func_decl("stream", "value")
        declaraion = Source(
            Location.DECLARATION,
            [
                [f"{load_decl};"],
                [f"{store_decl};"],
            ],
        )
        return Source(
            Location.DEFINITION,
            [
                [
                    f"{load_decl} {{",
                    f"    {self.cpp_type()} value = 0;",
                    *indent(try_unwrap(stream_read("stream", "&value", self.size()))),
                    *indent(self._extend_sign("value") if self.signed else []),
                    f"    return Ok(value);",
                    f"}}",
                ],
                [
                    f"{store_decl} {{",
                    *indent(self._check_bounds("value")),
                    *indent(try_unwrap(stream_write("stream", "&value", self.size()))),
                    f"    return Ok({monostate()});",
                    f"}}",
                ],
            ],
            deps=[declaraion],
        )

    def cpp_load(self, stream: str) -> str:
        if self.trivial:
            return super().cpp_load(stream)
        return self._cpp_load_func(stream)

    def cpp_store(self, stream: str, value: str) -> str:
        if self.trivial:
            return super().cpp_store(stream, value)
        return self._cpp_store_func(stream, value)

    def cpp_object(self, value: int) -> str:
        return self._int_literal(value, self.bits, self.signed)

    def pyi_type(self) -> str:
        return "int"


@dataclass
class Float(Type[float]):
    bits: int

    def __post_init__(self) -> None:
        super().__init__(sized=True, trivial=True)

    def name(self) -> Name:
        return Name(f"float{self.bits}")

    def size(self) -> int:
        return (self.bits - 1) // 8 + 1

    def load(self, data: bytes) -> float:
        assert len(data) == self.bits // 8
        if self.bits == 32:
            value = struct.unpack("<f", data)[0]
            assert isinstance(value, float)
            return value
        elif self.bits == 64:
            value = struct.unpack("<d", data)[0]
            assert isinstance(value, float)
            return value
        else:
            raise RuntimeError(f"{self.bits}-bit float is not supported")

    def store(self, value: float) -> bytes:
        if self.bits == 32:
            return struct.pack("<f", value)
        elif self.bits == 64:
            return struct.pack("<d", value)
        else:
            raise RuntimeError(f"{self.bits}-bit float is not supported")

    def default(self) -> float:
        return 0.0

    def random(self, rng: Random) -> float:
        return rng.gauss(0.0, 1.0)

    def is_instance(self, value: float) -> bool:
        return isinstance(value, float)

    def c_type(self) -> str:
        if self.bits == 32:
            return "float"
        elif self.bits == 64:
            return "double"
        else:
            raise RuntimeError(f"{self.bits}-bit float is not supported")

    def cpp_object(self, value: float) -> str:
        return f"{value}{'f' if self.bits == 32 else ''}"

    def pyi_type(self) -> str:
        return "float"


class Char(Type[str]):

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

    def cpp_object(self, value: str) -> str:
        assert len(value) == 1
        return f"'{value}'"

    def pyi_type(self) -> str:
        return "str"


@dataclass
class Pointer(Type[None]):
    type: Type[Any]
    const: bool = False
    _sep: str = "*"
    _postfix: str = "ptr"

    def __post_init__(self) -> None:
        super().__init__(sized=True, trivial=True)

    def name(self) -> Name:
        return Name(self.type.name(), "const" if self.const else "", self._postfix)

    def _ptr_type(self, type_str: str) -> str:
        return f"{'const ' if self.const else ''}{type_str} {self._sep}"

    def c_type(self) -> str:
        return self._ptr_type(self.type.c_type())

    def cpp_type(self) -> str:
        return self._ptr_type(self.type.cpp_type())

    def c_source(self) -> Optional[Source]:
        return self.type.c_source()

    def cpp_source(self) -> Optional[Source]:
        return self.type.cpp_source()


class Reference(Pointer):

    def __init__(self, type: Type[Any], const: bool = False):
        super().__init__(type, const, _sep="&", _postfix="ref")
