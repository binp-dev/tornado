from __future__ import annotations
from typing import List, Set

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
