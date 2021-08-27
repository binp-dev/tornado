from __future__ import annotations
from dataclasses import dataclass
from typing import List, Set


class Source:
    def __init__(self, items: List[str] = None, deps: List[Source] = []):
        self.items = items or []
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
    def __init__(self):
        pass

    def name(self) -> str:
        raise NotImplementedError()

    def c_type(self) -> str:
        raise NotImplementedError()

    def cpp_type(self) -> str:
        return self.c_type()

    def c_source(self) -> Source:
        return None

    def cpp_source(self) -> Source:
        return self.c_source()

    def c_len(self, obj: str) -> str:
        raise NotImplementedError()

    def c_load(self, dst: str, src: str, max_len: str) -> str:
        raise NotImplementedError()

    def c_store(self, src: str, dst: str) -> str:
        raise NotImplementedError()
