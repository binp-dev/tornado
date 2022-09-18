from __future__ import annotations
from typing import Any, List, Optional, Union

from random import Random
from functools import reduce
from operator import mul

import numpy as np
from numpy.typing import NDArray, DTypeLike

from ferrite.protogen.base import CONTEXT, Location, Name, Type, Source, UnexpectedEof
from ferrite.protogen.primitive import Char, Int
from ferrite.protogen.utils import flatten

_ArrayInstance = Union[List[Any], NDArray[Any]]


class _Sequence:

    def __init__(self, item: Type) -> None:
        assert item.is_sized()
        self.item = item

    def c_check(self, var: str, obj: _ArrayInstance) -> List[str]:
        return flatten([self.item.c_check(f"{var}.data[{i}]", x) for i, x in enumerate(obj)])

    def rust_check(self, var: str, obj: _ArrayInstance) -> List[str]:
        return flatten([self.item.rust_check(f"(&{var}[{i}])", x) for i, x in enumerate(obj)])

    def rust_object(self, obj: _ArrayInstance) -> str:
        return "[" + ", ".join([self.item.rust_object(x) for x in obj]) + "]"


class _ArrayLike(_Sequence, Type):

    def is_np(self) -> bool:
        return self.item.is_np()

    def np_dtype(self) -> DTypeLike:
        return self.item.np_dtype()

    def _load_array(self, data: bytes, count: int) -> _ArrayInstance:
        if len(data) < self.item.size * count:
            raise UnexpectedEof(self, data)
        if not self.is_np():
            array = []
            for i in range(count):
                array.append(self.item.load(data[(i * self.item.size):((i + 1) * self.item.size)]))
            return array
        else:
            shape = [count, *self.item.np_shape()]
            size = reduce(mul, shape)
            return np.frombuffer(data, self.np_dtype(), size).reshape(shape)

    def _store_array(self, array: _ArrayInstance) -> bytes:
        if not self.is_np():
            assert isinstance(array, list)
            data = b''
            for item in array:
                data += self.item.store(item)
            return data
        else:
            assert isinstance(array, np.ndarray) and array.dtype == self.np_dtype()
            return array.copy(order='C').tobytes()

    def _random_array(self, rng: Random, size: int) -> _ArrayInstance:
        array = [self.item.random(rng) for _ in range(size)]
        if not self.is_np():
            return array
        else:
            return np.array(array, dtype=self.np_dtype())

    def is_instance(self, value: Any) -> bool:
        if not self.is_np():
            return isinstance(value, list) and (len(value) == 0 or self.item.is_instance(value[0]))
        else:
            return isinstance(value, np.ndarray) and value.dtype == self.np_dtype()

    def _c_len(self, obj: str) -> str:
        raise NotImplementedError()

    def pyi_type(self) -> str:
        return f"List[{self.item.pyi_type()}]"

    def _pyi_np_dtype(self) -> str:
        return self.item._pyi_np_dtype()

    def pyi_source(self) -> Optional[Source]:
        return Source(Location.IMPORT, [["from typing import List"]], deps=[self.item.pyi_source()])


class Array(_ArrayLike):

    def __init__(self, item: Type, len: int) -> None:
        _Sequence.__init__(self, item)
        Type.__init__(self, Name(f"array{len}", item.name), self.item.size * len)
        self.len = len

    def load(self, data: bytes) -> _ArrayInstance:
        return self._load_array(data, self.len)

    def store(self, array: _ArrayInstance) -> bytes:
        assert len(array) == self.len
        return self._store_array(array)

    def random(self, rng: Random) -> _ArrayInstance:
        return self._random_array(rng, self.len)

    def is_instance(self, value: _ArrayInstance) -> bool:
        return len(value) == self.len and super().is_instance(value)

    def np_shape(self) -> List[int]:
        return [self.len, *self.item.np_shape()]

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name).camel()

    def _c_len(self, obj: str) -> str:
        return f"((size_t){self.len})"

    def c_source(self) -> Source:
        return Source(
            Location.DECLARATION,
            [[
                f"typedef struct __attribute__((packed, aligned(1))) {{",
                f"    {self.item.c_type()} data[{self.len}];",
                f"}} {self.c_type()};",
            ]],
            deps=[self.item.c_source()],
        )

    def rust_type(self) -> str:
        return f"[{self.item.rust_type()}; {self.len}]"


class _VectorLike(_Sequence, Type):

    def __init__(self, name: Name, item: Type) -> None:
        size_type = Int(16)
        _Sequence.__init__(self, item)
        Type.__init__(self, name, None, size_type.size)
        self._len_type = size_type

    def c_type(self) -> str:
        return Name(CONTEXT.prefix, self.name).camel()

    def c_size(self, obj: str) -> str:
        return f"((size_t){self.min_size} + {self._c_size_extent(obj)})"

    def _c_size_extent(self, obj: str) -> str:
        return f"({self._c_len(obj)} * {self.item.size})"

    def _c_len(self, obj: str) -> str:
        return f"(size_t){obj}.len"

    def c_source(self) -> Source:
        name = self.c_type()
        return Source(
            Location.DECLARATION,
            [[
                f"typedef struct __attribute__((packed, aligned(1))) {{",
                f"    {self._len_type.c_type()} len;",
                f"    {self.item.c_type()} data[];",
                f"}} {name};",
            ]],
            deps=[
                self.item.c_source(),
                self._len_type.c_source(),
            ],
        )

    def rust_type(self) -> str:
        return f"FlatVec<{self.item.rust_type()}, {self._len_type.rust_type()}>"

    def rust_source(self) -> Source:
        return Source(
            Location.IMPORT,
            [["use flatty::FlatVec;"]],
            deps=[
                self.item.rust_source(),
                self._len_type.rust_source(),
            ],
        )

    def c_check(self, var: str, obj: _ArrayInstance) -> List[str]:
        return [
            f"codegen_assert_eq({var}.len, {len(obj)});",
            *super().c_check(var, obj),
        ]

    def rust_check(self, var: str, obj: _ArrayInstance) -> List[str]:
        return [
            f"assert_eq!({var}.len(), {len(obj)});",
            *super().rust_check(var, obj),
        ]

    def rust_object(self, obj: _ArrayInstance) -> str:
        return "vec!" + super().rust_object(obj)


class Vector(_VectorLike, _ArrayLike):

    def __init__(self, item: Type):
        super().__init__(Name("vector", item.name), item)

    def load(self, data: bytes) -> _ArrayInstance:
        count = self._len_type.load(data[:self._len_type.size])
        data = data[self.min_size:]
        return self._load_array(data, count)

    def store(self, array: _ArrayInstance) -> bytes:
        data = self._len_type.store(len(array))
        data += self._store_array(array)
        return data

    def size_of(self, value: _ArrayInstance) -> int:
        return self._len_type.size + self.item.size * len(value)

    def random(self, rng: Random) -> _ArrayInstance:
        size = rng.randrange(0, 8)
        return self._random_array(rng, size)


class String(_VectorLike):

    def __init__(self) -> None:
        super().__init__(Name("string"), Char())

    def load(self, data: bytes) -> str:
        count = self._len_type.load(data[:self._len_type.size])
        data = data[self._len_type.size:]
        if len(data) < count:
            raise UnexpectedEof(self, data)
        return data.decode("ascii")

    def store(self, value: str) -> bytes:
        data = b''
        data += self._len_type.store(len(value))
        data += value.encode("ascii")
        return data

    def size_of(self, value: str) -> int:
        return self._len_type.size + self.item.size * len(value)

    def random(self, rng: Random) -> str:
        size = rng.randrange(0, 64)
        return "".join([Char().random(rng) for _ in range(size)])

    def is_instance(self, value: Any) -> bool:
        return isinstance(value, str)

    def pyi_type(self) -> str:
        return f"str"
