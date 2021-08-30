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

class Type:
    def __init__(self, sized: bool = False):
        self.sized = sized

    def name(self) -> Name:
        raise NotImplementedError()

    def min_size(self) -> int:
        raise NotImplementedError()

    def c_type(self) -> str:
        raise NotImplementedError()

    def cpp_type(self) -> str:
        return self.c_type()

    def c_source(self) -> Source:
        return None

    def cpp_source(self) -> Source:
        return self.c_source()

    def c_size(self, obj: str) -> str:
        raise NotImplementedError()

class Sized(Type):
    def __init__(self):
        super().__init__(sized=True)

    def size(self) -> int:
        raise NotImplementedError()

    def min_size(self) -> int:
        return self.size()
