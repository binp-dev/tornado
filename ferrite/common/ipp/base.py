from __future__ import annotations
from typing import List, Set, Union

class Name:
    def __init__(self, *args):
        self.words = []
        for arg in args:
            if isinstance(arg, Name):
                self.words += arg.words
            elif isinstance(arg, list):
                self.words += arg
            elif isinstance(arg, str):
                self.words.append(arg)
            else:
                raise RuntimeError(f"Unsupported argument {type(arg).__name__}")

    def camel(self):
        return "".join([s[0].upper() + s[1:].lower() for s in self.words])

    def snake(self):
        return "_".join([s.lower() for s in self.words])

    @staticmethod
    def from_snake(snake: str) -> Name:
        Name(snake.split("_"))

class Source:
    def __init__(self, items: Union[List[str], str] = None, deps: List[Source] = []):
        if items is not None:
            if isinstance(items, str):
                self.items = [items]
            elif isinstance(items, list):
                self.items = items
            else:
                raise RuntimeError(f"Unsupported type {type(items).__name__}")
        else:
            self.items = []

        self.deps = [p for p in deps if p is not None]

    def collect(self, used: Set[str] = None) -> List[str]:
        if used is None:
            used = set()

        result = []

        for dep in self.deps:
            result.extend(dep.collect(used))

        for item in self.items:
            if item is not None and item not in used:
                result.append(item)
                used.add(item)

        return result

    def make_source(self) -> str:
        return "\n\n".join(self.collect()) + "\n"

class Include(Source):
    def __init__(self, path):
        super().__init__([f"#include <{path}>"])

class CType:
    def __init__(self, prefix: str, postfix: str = None):
        self.prefix = prefix
        self.postfix = postfix
    
    def __str__(self) -> str:
        if self.postfix is not None:
            raise RuntimeError("CType has a postfix")
        return self.prefix
    
    def declare(self, variable: str) -> str:
        return f"{self.prefix} {variable}{self.postfix or ''}"

def declare_variable(c_type: Union[CType, str], variable: str) -> str:
    if isinstance(c_type, CType):
        return c_type.declare(variable)
    else:
        return f"{c_type} {variable}"

class Type:
    def __init__(self, sized: bool = False):
        self.sized = sized

    def name(self) -> Name:
        raise NotImplementedError()

    def min_size(self) -> int:
        raise NotImplementedError()

    def c_type(self) -> Union[CType, str]:
        raise NotImplementedError()

    def cpp_type(self) -> Union[CType, str]:
        return self.c_type()

    def c_source(self) -> Source:
        return None

    def cpp_source(self) -> Source:
        return self.c_source()

    def c_size(self, obj: str) -> str:
        raise NotImplementedError()

    def _c_size_extent(self, obj: str) -> str:
        raise NotImplementedError()

    def cpp_size(self, obj: str) -> str:
        return self.c_size(obj)

    def cpp_load(self, dst: str, src: str) -> str:
        raise NotImplementedError()
    
    def cpp_store(self, dst: str, src: str) -> str:
        raise NotImplementedError()

class SizedType(Type):
    def __init__(self):
        super().__init__(sized=True)

    def size(self) -> int:
        raise NotImplementedError()

    def min_size(self) -> int:
        return self.size()

    def c_size(self, obj: str) -> str:
        return str(self.size())

    def _c_size_extent(self, obj: str) -> str:
        raise NotImplementedError()