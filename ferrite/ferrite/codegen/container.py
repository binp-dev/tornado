from __future__ import annotations
from typing import Any, List, Optional

from random import Random
from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray, DTypeLike

from ferrite.codegen.base import CONTEXT, Include, Location, Name, Type, Source
from ferrite.codegen.primitive import Char, Int
from ferrite.codegen.utils import indent


class _ItemBase(Type):

    def __init__(self, item: Type, sized: bool) -> None:
        assert item.sized
        self.item = item
        super().__init__(sized=sized)

    def deps(self) -> List[Type]:
        return [self.item]


class _ArrayBase(_ItemBase):

    def _is_np(self) -> bool:
        return self.item.trivial or (isinstance(self.item, _ArrayBase) and self.item._is_np())

    def np_dtype(self) -> DTypeLike:
        if self._is_np():
            return self.item.np_dtype()
        else:
            raise NotImplementedError()

    def pyi_np_dtype(self) -> str:
        if self._is_np():
            return self.item.pyi_np_dtype()
        else:
            raise NotImplementedError()

    def _load_array(self, data: bytes, size: int) -> List[Any] | NDArray[Any]:
        item_size = self.item.size()
        assert len(data) == item_size * size
        if not self._is_np():
            array = []
            for i in range(size):
                array.append(self.item.load(data[(i * item_size):((i + 1) * item_size)]))
            return array
        else:
            return np.frombuffer(data, self.np_dtype(), size)

    def _store_array(self, array: List[Any] | NDArray[Any]) -> bytes:
        if not self._is_np():
            assert isinstance(array, list)
            data = b''
            for item in array:
                data += self.item.store(item)
            return data
        else:
            assert isinstance(array, np.ndarray) and array.dtype == self.np_dtype()
            return array.copy(order='C').tobytes()

    def _random_array(self, rng: Random, size: int) -> List[Any] | NDArray[Any]:
        array = [self.item.random(rng) for _ in range(size)]
        if not self._is_np():
            return array
        else:
            return np.array(array, dtype=self.np_dtype())

    def is_instance(self, value: List[Any] | NDArray[Any]) -> bool:
        if not self._is_np():
            return isinstance(value, list)
        else:
            return isinstance(value, np.ndarray) and value.dtype == self.np_dtype()

    def c_len(self, obj: str) -> str:
        raise NotImplementedError()

    def pyi_type(self) -> str:
        if not self._is_np():
            return f"List[{self.item.pyi_type()}]"
        else:
            return f"NDArray[{self.pyi_np_dtype()}]"

    def pyi_source(self) -> Optional[Source]:
        if not self._is_np():
            imports = [["from typing import List"]]
        else:
            imports = [["import numpy as np"], ["from numpy.typing import NDArray"]]
        return Source(Location.INCLUDES, imports)


class Array(_ArrayBase):

    def __init__(self, item: Type, len: int) -> None:
        super().__init__(item, sized=True)
        self.len = len

    def name(self) -> Name:
        return Name(f"array{self.len}", self.item.name())

    def size(self) -> int:
        return self.item.size() * self.len

    def load(self, data: bytes) -> List[Any] | NDArray[Any]:
        return self._load_array(data, self.len)

    def store(self, array: List[Any] | NDArray[Any]) -> bytes:
        assert len(array) == self.len
        return self._store_array(array)

    def random(self, rng: Random) -> List[Any] | NDArray[Any]:
        return self._random_array(rng, self.len)

    def is_instance(self, value: List[Any] | NDArray[Any]) -> bool:
        return len(value) == self.len and super().is_instance(value)

    def c_size(self, obj: str) -> str:
        return str(self.size())

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name()).camel()

    def rust_type(self) -> str:
        if self.len is not None:
            return f"[{self.item.rust_type()}; {self.len}]"
        else:
            return f"[{self.item.rust_type()}]"

    def c_source(self) -> Source:
        name = self.c_type()
        return Source(
            Location.DECLARATION,
            [[
                f"typedef struct {{",
                f"    {self.item.c_type()} data[{self.len}];",
                f"}} {name};",
            ]],
            deps=[self.item.c_source()],
        )

    def c_len(self, obj: str) -> str:
        return f"size_t({self.len})"


@dataclass
class _BasicVector(_ItemBase):

    def __init__(self, item: Type) -> None:
        super().__init__(item, sized=False)
        self._size_type = Int(16)

    def name(self) -> Name:
        return Name("vector", self.item.name())

    def min_size(self) -> int:
        return self._size_type.size()

    def deps(self) -> List[Type]:
        return [self.item, self._size_type]

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name()).camel()

    def rust_type(self) -> str:
        return f"FlatVec<{self.item.rust_type()}, {self._size_type.rust_type()}>"

    def c_source(self) -> Source:
        name = self.c_type()
        return Source(
            Location.DECLARATION,
            [[
                f"typedef struct {{",
                f"    {self._size_type.c_type()} len;",
                f"    {self.item.c_type()} data[];",
                f"}} {name};",
            ]],
            deps=[
                self.item.c_source(),
                self._size_type.c_source(),
            ],
        )

    def rust_source(self) -> Source:
        return Source(
            Location.INCLUDES,
            [["use flatty::FlatVec;"]],
            deps=[
                self.item.rust_source(),
                self._size_type.rust_source(),
            ],
        )

    def c_size(self, obj: str) -> str:
        return f"((size_t){self.min_size()} + ({obj}.len * {self.item.size()}))"

    def _c_size_extent(self, obj: str) -> str:
        item_size = self.item.size()
        return f"((size_t){obj}.len{f' * {item_size}' if item_size != 1 else ''})"

    def c_len(self, obj: str) -> str:
        return f"{obj}.len"


class Vector(_BasicVector, _ArrayBase):

    def __init__(self, item: Type):
        super().__init__(item)

    def load(self, data: bytes) -> List[Any] | NDArray[Any]:
        count = self._size_type.load(data[:self._size_type.size()])
        data = data[self._size_type.size():]
        return self._load_array(data, count)

    def store(self, array: List[Any] | NDArray[Any]) -> bytes:
        return self._size_type.store(len(array)) + self._store_array(array)

    def random(self, rng: Random) -> List[Any] | NDArray[Any]:
        size = rng.randrange(0, 8)
        return self._random_array(rng, size)


class String(_BasicVector):

    def __init__(self) -> None:
        super().__init__(Char())

    def name(self) -> Name:
        return Name("string")

    def load(self, data: bytes) -> str:
        count = self._size_type.load(data[:self._size_type.size()])
        data = data[self._size_type.size():]
        assert len(data) == count
        return data.decode("ascii")

    def store(self, value: str) -> bytes:
        data = b''
        data += self._size_type.store(len(value))
        data += value.encode("ascii")
        return data

    def random(self, rng: Random) -> str:
        size = rng.randrange(0, 64)
        return "".join([Char().random(rng) for _ in range(size)])

    def is_instance(self, value: Any) -> bool:
        return isinstance(value, str)

    def pyi_type(self) -> str:
        return f"str"
