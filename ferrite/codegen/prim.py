from __future__ import annotations
from random import Random
from typing import Any
from dataclasses import dataclass
import string
import struct

from codegen.base import CONTEXT, Location, Name, TrivialType, SizedType, Type, Source
from codegen.util import ceil_to_power_of_2, is_power_of_2

@dataclass
class Int(SizedType):
    bits: int
    signed: bool = False

    def _is_builtin(self):
        return is_power_of_2(self.bits // 8) and (self.bits % 8) == 0

    def __post_init__(self):
        super().__init__(trivial=self._is_builtin())

    def name(self) -> Name:
        return Name(("u" if not self.signed else "") + "int" + str(self.bits))

    def size(self) -> int:
        return (self.bits - 1) // 8 + 1

    def load(self, data: bytes) -> int:
        assert len(data) == self.bits // 8
        return int.from_bytes(data, byteorder="little", signed=self.signed)
    
    def store(self, value: int) -> bytes:
        return value.to_bytes(self.bits // 8, byteorder="little", signed=self.signed)

    def random(self, rng: Random) -> int:
        if not self.signed:
            return rng.randrange(0, 2**self.bits)
        else:
            return rng.randrange(-2**(self.bits - 1), 2**(self.bits - 1))

    def is_instance(self, value: Any) -> bool:
        return isinstance(value, int)

    @staticmethod
    def _int_type(bits: int, signed: bool = False) -> str:
        return f"{'u' if not signed else ''}int{bits}_t"

    @staticmethod
    def _int_literal(value: int, bits: int, signed: bool = False) -> str:
        return f"{value}{'u' if not signed else ''}{'ll' if bits > 32 else ''}"

    def c_type(self) -> str:
        ident = self._int_type(self.bits, self.signed)
        if not self._is_builtin() and CONTEXT.prefix is not None:
            ident = CONTEXT.prefix + "_" + ident
        return ident

    def c_source(self) -> Source:
        if self.bits % 8 != 0 or self.bits > 64:
            raise RuntimeError(f"{self.bits}-bit integer is not supported")
        bytes = self.bits // 8
        if self._is_builtin():
            return None
        else:
            if self.signed:
                raise RuntimeError(f"Signed integers are only supported to have power-of-2 size")
            name = self.c_type()
            ceil_name = self._int_type(ceil_to_power_of_2(self.bits))
            prefix = f"{CONTEXT.prefix}_" if CONTEXT.prefix is not None else ""
            load_decl = f"{ceil_name} {prefix}uint{self.bits}_load({name} x)"
            store_decl = f"{name} {prefix}uint{self.bits}_store({ceil_name} y)"
            declaraion = Source(Location.DECLARATION, "\n".join([
                f"typedef struct {name} {{",
                f"    uint8_t bytes[{bytes}];",
                f"}} {name};",
                f"",
                f"{load_decl};"
                f"{store_decl};"
            ]))
            return Source(Location.DEFINITION, "\n".join([
                f"{load_decl} {{",
                f"    {ceil_name} y = 0;",
                f"    memcpy((void *)&y, (const void *)&x, {self.size()});",
                f"    return y;",
                f"}}",
                f"",
                f"{store_decl} {{",
                f"    {name} x;",
                f"    memcpy((void *)&x, (const void *)&y, {self.size()});",
                f"    return x;",
                f"}}",
            ]), deps=[declaraion])

    def cpp_type(self) -> str:
        return self._int_type(ceil_to_power_of_2(self.bits), self.signed)

    def cpp_source(self) -> Source:
        return None

    def cpp_load(self, src: str) -> str:
        if self._is_builtin():
            return f"{src}"
        else:
            prefix = f"{CONTEXT.prefix}_" if CONTEXT.prefix is not None else ""
            return f"{prefix}uint{self.bits}_load({src})"

    def cpp_store(self, src: str, dst: str) -> str:
        if self._is_builtin():
            return f"{dst} = {src}"
        else:
            prefix = f"{CONTEXT.prefix}_" if CONTEXT.prefix is not None else ""
            return f"{dst} = {prefix}uint{self.bits}_store({src})"

    def cpp_object(self, value: int) -> str:
        return self._int_literal(value, self.bits, self.signed)

    def c_test(self, obj: str, src: str) -> str:
        return TrivialType.c_test(self, obj, src)

    def cpp_test(self, dst: str, src: str) -> str:
        return TrivialType.cpp_test(self, dst, src)

    def test_source(self) -> Source:
        if self._is_builtin():
            return None
        else:
            return super().test_source()

@dataclass
class Float(TrivialType):
    bits: int

    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name(f"float{self.bits}")

    def size(self) -> int:
        return (self.bits - 1) // 8 + 1

    def load(self, data: bytes) -> float:
        assert len(data) == self.bits // 8
        if self.bits == 32:
            return struct.unpack("<f", data)[0]
        elif self.bits == 64:
            return struct.unpack("<d", data)[0]
        else:
            raise RuntimeError(f"{self.bits}-bit float is not supported")
    
    def store(self, value: float) -> bytes:
        if self.bits == 32:
            return struct.pack("<f", value)
        elif self.bits == 64:
            return struct.pack("<d", value)
        else:
            raise RuntimeError(f"{self.bits}-bit float is not supported")

    def random(self, rng: Random) -> float:
        return rng.gauss(0.0, 1.0)

    def is_instance(self, value: Any) -> bool:
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

@dataclass
class Char(TrivialType):
    def __post_init__(self):
        super().__init__()

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

    def is_instance(self, value: Any) -> bool:
        return isinstance(value, str) and len(value) == 1

    def c_type(self) -> str:
        return "char"

    def cpp_object(self, value: str) -> str:
        assert len(value) == 1
        return f"'{value}'"

@dataclass
class Size(SizedType):
    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name("usize")

    def c_type(self) -> str:
        return "size_t"

@dataclass
class Pointer(SizedType):
    type: Type
    const: bool = False
    _sep: str = "*"
    _postfix: str = "ptr"

    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name(self.type.name(), "const" if self.const else "", self._postfix)

    def _ptr_type(self, type_str: str) -> str:
        return f"{'const ' if self.const else ''}{type_str} {self._sep}"

    def c_type(self) -> str:
        return self._ptr_type(self.type.c_type())
    
    def cpp_type(self) -> str:
        return self._ptr_type(self.type.cpp_type())

    def c_source(self) -> Source:
        return self.type.c_source()

    def cpp_source(self) -> Source:
        return self.type.cpp_source()

class Reference(Pointer):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs, _sep="&", _postfix="ref")
