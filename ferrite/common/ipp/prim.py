from __future__ import annotations
from random import Random
from typing import List
from dataclasses import dataclass

from ipp.base import CONTEXT, CType, Location, Name, TestInfo, TestSource, TrivialType, SizedType, Type, Source
from ipp.util import ceil_to_power_of_2, is_power_of_2

@dataclass
class Int(SizedType):
    bits: int
    signed: bool = False

    def _is_trivial(self):
        return is_power_of_2(self.bits // 8) and (self.bits % 8) == 0

    def __post_init__(self):
        super().__init__(trivial=self._is_trivial())

    def name(self) -> Name:
        return Name(("u" if not self.signed else "") + "int" + str(self.bits))

    def size(self) -> int:
        return (self.bits - 1) // 8 + 1

    @staticmethod
    def _int_type(bits: int, signed: bool = False) -> str:
        return f"{'u' if not signed else ''}int{bits}_t"

    def c_type(self) -> str:
        ident = self._int_type(self.bits, self.signed)
        if not self._is_trivial() and CONTEXT.prefix is not None:
            ident = CONTEXT.prefix + "_" + ident
        return ident

    def c_source(self) -> Source:
        if self.bits % 8 != 0 or self.bits > 64:
            raise RuntimeError(f"{self.bits}-bit integer is not supported")
        bytes = self.bits // 8
        if self._is_trivial():
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
        return self._int_type(ceil_to_power_of_2(self.bits))

    def cpp_source(self) -> Source:
        return self.cpp_test(TestInfo()).source()

    def cpp_load(self, src: str) -> str:
        if self._is_trivial():
            return f"{src}"
        else:
            prefix = f"{CONTEXT.prefix}_" if CONTEXT.prefix is not None else ""
            return f"{prefix}uint{self.bits}_load({src})"

    def cpp_store(self, src: str, dst: str) -> str:
        if self._is_trivial():
            return f"{dst} = {src}"
        else:
            prefix = f"{CONTEXT.prefix}_" if CONTEXT.prefix is not None else ""
            return f"{dst} = {prefix}uint{self.bits}_store({src})"

    def cpp_test(self, info: TestInfo) -> TestSource:
        if not self.signed:
            value = info.rng.randrange(0, 2**self.bits)
        else:
            value = info.rng.randrange(-2**(self.bits - 1), 2**(self.bits - 1))
        
        return TestSource(
            type=self,
            cpp_create=f"{info.cpp_create_prefix}{value}",
            c_content=f"EXPECT_EQ({self.cpp_load(info.c_content_prefix)}, {value});",
            cpp_content=f"EXPECT_EQ({info.cpp_content_prefix}, {value});",
        )

@dataclass
class Float(TrivialType):
    bits: int

    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name(f"float{self.bits}")

    def size(self) -> int:
        return (self.bits - 1) // 8 + 1

    def c_type(self) -> str:
        if self.bits == 32:
            return "float"
        elif self.bits == 64:
            return "double"
        else:
            raise RuntimeError(f"{self.bits}-bit float is not supported")

@dataclass
class Char(TrivialType):
    def __post_init__(self):
        super().__init__()

    def name(self) -> Name:
        return Name("char")

    def size(self) -> int:
        return 1

    def c_type(self) -> str:
        return "char"

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

@dataclass
class Array(Type):
    type: SizedType
    len: int

    def __post_init__(self):
        assert self.type.sized
        super().__init__(
            sized=self.len is not None,
            trivial=self.type.trivial,
        )

    def size(self) -> int:
        if self.len is not None:
            return self.type.size() * self.len
        else:
            raise NotImplementedError()

    def min_size(self) -> int:
        if self.len is not None:
            return self.size()
        else:
            return 0

    def c_size(self, obj: str) -> str:
        return str(self.size())

    def c_type(self) -> CType:
        return CType(str(self.type.c_type()), f"[{self.len if self.len is not None else ''}]")

    def c_source(self) -> Source:
        return self.type.c_source()

    def cpp_source(self) -> Source:
        return self.type.cpp_source()
