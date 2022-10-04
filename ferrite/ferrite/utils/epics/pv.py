from __future__ import annotations
from typing import Any, AsyncGenerator, AsyncIterator, Generic, List, Literal, Optional, Type, TypeVar, overload, Union

from enum import Enum
from contextlib import asynccontextmanager
import asyncio

import pyepics_asyncio as aepics

import numpy as np
from numpy.typing import NDArray

from ferrite.utils.asyncio.task import with_timeout

T = TypeVar("T", bool, int, float, str, NDArray[np.int32], NDArray[np.float64])


class PvType(Enum):
    BOOL = 0
    INT = 1
    FLOAT = 2
    STR = 3
    ARRAY_INT = 4
    ARRAY_FLOAT = 5

    def _type(self) -> Type[_PvAny]:
        if self == PvType.BOOL:
            return _PvBool
        elif self == PvType.INT:
            return _PvInt
        elif self == PvType.FLOAT:
            return _PvFloat
        elif self == PvType.STR:
            return _PvStr
        elif self == PvType.ARRAY_INT:
            return _PvArrayInt
        elif self == PvType.ARRAY_FLOAT:
            return _PvArrayFloat
        else:
            raise RuntimeError("Unreachable")

    def _check_pv(self, raw: aepics.Pv) -> None:
        int_names = ["char", "short", "int", "long", "enum"]
        float_names = ["float", "double"]

        rtype = raw.raw.type
        try:
            if self == PvType.BOOL:
                assert rtype in int_names
                assert raw.nelm == 1
            elif self == PvType.INT:
                assert rtype in int_names
                assert raw.nelm == 1
            elif self == PvType.FLOAT:
                assert rtype in float_names
                assert raw.nelm == 1
            elif self == PvType.STR:
                assert rtype == "string"
                assert raw.nelm == 1
            elif self == PvType.ARRAY_INT:
                assert rtype in int_names
                assert raw.nelm > 1
            elif self == PvType.ARRAY_FLOAT:
                assert rtype in float_names
                assert raw.nelm > 1
            else:
                raise RuntimeError("Unreachable")
        except AssertionError:
            raise AssertionError(
                f"Actual PV '{raw.name}' type '{raw.raw.type}' (nelm: {raw.nelm}) does not match requested type {self}"
            )


class Pv(Generic[T]):
    type: PvType

    def __init__(self, raw: aepics.Pv) -> None:
        self.type._check_pv(raw)
        self._raw = raw

    @staticmethod
    def _convert(value: Any) -> T:
        raise NotImplementedError()

    async def get(self) -> T:
        return self._convert(await self._raw.get())

    async def put(self, value: T) -> None:
        await self._raw.put(value)

    async def _generator(self, mon: AsyncGenerator[Any, None]) -> AsyncGenerator[T, None]:
        async for value in mon:
            yield self._convert(value)

    @asynccontextmanager
    async def monitor(self, current: bool = False) -> AsyncIterator[AsyncGenerator[T, None]]:
        async with self._raw.monitor(current=current) as mon:
            yield self._generator(mon)

    @property
    def name(self) -> str:
        return self._raw.name


class PvArray(Pv[T]):

    @property
    def nelm(self) -> int:
        return self._raw.nelm


class _PvBool(Pv[bool]):

    type: PvType = PvType.BOOL

    @staticmethod
    def _convert(value: Any) -> bool:
        assert isinstance(value, int) and value in [0, 1]
        return bool(value)


class _PvInt(Pv[int]):

    type: PvType = PvType.INT

    @staticmethod
    def _convert(value: Any) -> int:
        assert isinstance(value, int)
        return int(value)


class _PvFloat(Pv[float]):

    type: PvType = PvType.FLOAT

    @staticmethod
    def _convert(value: Any) -> float:
        assert isinstance(value, float)
        return float(value)


class _PvStr(Pv[str]):

    type: PvType = PvType.STR

    @staticmethod
    def _convert(value: Any) -> str:
        assert isinstance(value, str)
        return str(value)


class _PvArrayInt(PvArray[NDArray[np.int32]]):

    type: PvType = PvType.ARRAY_INT

    @staticmethod
    def _convert(array: Any) -> NDArray[np.int32]:
        try:
            assert isinstance(array, np.ndarray)
        except AssertionError:
            assert isinstance(array, int)
            return np.array([array], dtype=np.int32)
        else:
            assert array.dtype == np.int32
            return array


class _PvArrayFloat(PvArray[NDArray[np.float64]]):

    type: PvType = PvType.ARRAY_FLOAT

    @staticmethod
    def _convert(array: Any) -> NDArray[np.float64]:
        try:
            assert isinstance(array, np.ndarray)
        except AssertionError:
            assert isinstance(array, float)
            return np.array([array], dtype=np.float64)
        else:
            assert array.dtype == np.float64
            return array


_PvAny = Union[Pv[bool], Pv[int], Pv[float], Pv[str], PvArray[NDArray[np.int32]], PvArray[NDArray[np.float64]]]


class Context:

    @overload
    async def connect(self, name: str, pv_type: Literal[PvType.BOOL]) -> Pv[bool]:
        ...

    @overload
    async def connect(self, name: str, pv_type: Literal[PvType.INT]) -> Pv[int]:
        ...

    @overload
    async def connect(self, name: str, pv_type: Literal[PvType.FLOAT]) -> Pv[float]:
        ...

    @overload
    async def connect(self, name: str, pv_type: Literal[PvType.STR]) -> Pv[str]:
        ...

    @overload
    async def connect(self, name: str, pv_type: Literal[PvType.ARRAY_INT]) -> PvArray[NDArray[np.int32]]:
        ...

    @overload
    async def connect(self, name: str, pv_type: Literal[PvType.ARRAY_FLOAT]) -> PvArray[NDArray[np.float64]]:
        ...

    async def connect(self, name: str, pv_type: PvType, timeout: Optional[float] = 2.0) -> _PvAny:
        connect_future = aepics.Pv.connect(name)
        if timeout is not None:
            try:
                raw = await with_timeout(connect_future, timeout)
            except TimeoutError as e:
                raise TimeoutError(f"Cannot connect to PV '{name}': {e}")
        else:
            raw = await connect_future

        # Without this workaround calling `monitor()` right after `connect()` sometimes returns initial value.
        await asyncio.sleep(0.2)

        return pv_type._type()(raw)
