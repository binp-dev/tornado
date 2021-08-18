from __future__ import annotations
from typing import List, Set
from dataclasses import dataclass
import re
import zlib

def to_ident(text: str, _pat = re.compile("[^a-zA-Z0-9_]")):
    ident = re.sub(_pat, "_", text)
    if ident != text:
        hash = zlib.adler32(text.encode("utf-8"))
        ident += f"_{hash:0{8}x}"
    return ident

class Prelude:
    def __init__(self, text: str = None, deps: List[Prelude] = []):
        self.text = text
        self.deps = deps

    def collect(self, used: Set(str) = None) -> str:
        if used is None:
            used = set()

        result = []
        for dep in self.deps:
            result.append(dep.collect(used))
        if self.text is not None and self.text not in used:
            result.append(self.text)
            used.add(self.text)

        return "\n".join(result)

class Type:
    def __init__(self):
        pass

    def c_type(self) -> str:
        raise NotImplementedError

    def cpp_type(self) -> str:
        return self.c_type()

    def c_prelude(self) -> Prelude:
        return None

    def cpp_prelude(self) -> Prelude:
        return self.c_prelude()

@dataclass
class Field:
    name: str
    type: Type

class Struct(Type):
    def __init__(self, name: str, fields: List[Field] = []):
        self.name = name
        self.fields = fields

    def _c_definition(self) -> str:
        return "".join([s + "\n" for s in [
            f"typedef struct {self.name} {{",
            *[f"    {field.type.c_type()} {field.name};" for field in self.fields],
            f"}} {self.name};"
        ]])

    def _cpp_definition(self) -> str:
        return "".join([s + "\n" for s in [
            f"class {self.name} final {{",
            f"private:",
            *[f"    {field.type.cpp_type()} {field.name}_;" for field in self.fields],
            f"}};"
        ]])

    def c_type(self) -> str:
        return self.name
    
    def cpp_type(self) -> str:
        return self.name

    def c_prelude(self) -> Prelude:
        return Prelude(
            self._c_definition(),
            [p for p in [field.type.c_prelude() for field in self.fields] if p is not None],
        )
    
    def cpp_prelude(self) -> Prelude:
        return Prelude(
            self._cpp_definition(),
            [p for p in [field.type.cpp_prelude() for field in self.fields] if p is not None],
        )

class Variant(Type):
    def __init__(self, name: str, options: List[Field]):
        self.name = name
        self.options = options

    def _c_definition(self) -> str:
        return "".join([s + "\n" for s in [
            f"typedef struct {self.name} {{",
            f"    uint8_t type;",
            f"    union {{",
            *[f"        {option.type.c_type()} {option.name};" for option in self.options],
            f"    }};"
            f"}} {self.name};"
        ]])

    def _cpp_definition(self) -> str:
        return "".join([s + "\n" for s in [
            f"class {self.name} final {{",
            f"private:",
            f"    std::variant<",
            *[
                f"        {option.type.cpp_type()}{',' if i < len(self.options) else ''}"
                for i, option in enumerate(self.options)
            ],
            f"    > variant_;",
            f"}};"
        ]])

    def c_type(self) -> str:
        return self.name

    def cpp_type(self) -> str:
        return self.name

    def c_prelude(self) -> Prelude:
        return Prelude(
            self._c_definition(),
            [
                Prelude("#include <stdint.h>"),
                *[p for p in [option.type.c_prelude() for option in self.options] if p is not None],
            ],
        )

    def cpp_prelude(self) -> Prelude:
        return Prelude(
            self._cpp_definition(),
            [
                Prelude("#include <variant>"),
                *[p for p in [option.type.cpp_prelude() for option in self.options] if p is not None],
            ],
        )

@dataclass
class Pointer(Type):
    type: Type
    const: bool = False

    def _ptr_type(self, type_str: str) -> str:
        return f"{'const ' if self.const else ''}{type_str} *"

    def c_type(self) -> str:
        return self._ptr_type(self.type.c_type())
    
    def cpp_type(self) -> str:
        return self._ptr_type(self.type.cpp_type())

    def c_prelude(self) -> Prelude:
        return self.type.c_prelude()

    def cpp_prelude(self) -> Prelude:
        return self.type.cpp_prelude()

@dataclass
class Int(Type):
    size: int
    signed: bool = False

    def c_type(self) -> str:
        return f"{'u' if not self.signed else ''}int{self.size}_t"

    def c_prelude(self) -> Prelude:
        return Prelude("#include <stdint.h>")

    def cpp_prelude(self) -> Prelude:
        return Prelude("#include <cstdint>")

class Vector(Type):
    def __init__(self, item: Type):
        super().__init__()
        self.item = item
        self._c_struct = Struct(
            "Vector_" + to_ident(self.item.c_type()),
            [
                Field("len", Int(16)),
                Field("data", Pointer(item)),
            ],
        )

    def c_type(self) -> str:
        return self._c_struct.c_type()

    def cpp_type(self) -> str:
        return f"std::vector<{self.item.cpp_type()}>"

    def c_prelude(self) -> Prelude:
        return self._c_struct.c_prelude()

    def cpp_prelude(self) -> Prelude:
        return Prelude("#include <vector>", [self.item.cpp_prelude()])

class String(Type):
    def __init__(self):
        super().__init__()

    def c_type(self) -> str:
        return "char *"

    def cpp_type(self) -> str:
        return "std::string"

    def cpp_prelude(self) -> Prelude:
        return Prelude("#include <string>")
