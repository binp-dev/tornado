from __future__ import annotations
from typing import Any, List, Optional, Awaitable, AsyncIterator, Generic, Literal, Type, TypeVar, overload

from dataclasses import dataclass
from enum import IntEnum
import asyncio
import pyepics_asyncio as ae

import numpy as np
from numpy.typing import NDArray

T = TypeVar("T", bool, int, float, str, NDArray[np.int32], NDArray[np.float64])
S = TypeVar("S", bool, int, float, str)
A = TypeVar("A", NDArray[np.int32], NDArray[np.float64])

P = TypeVar("P", bound=ae.PvBase, covariant=True)


class PvBase(Generic[P, T]):

    def __init__(self, pv: P) -> None:
        self.pv = pv

    @property
    def name(self) -> str:
        return self.pv.name

    async def put(self, value: T) -> None:
        await self.pv.put(value)

    def _convert(self, value: Any) -> T:
        raise NotImplementedError()


class PvArrayBase(PvBase[P, A]):

    @property
    def nelm(self) -> int:
        return self.pv.nelm


class Pv(PvBase[ae.Pv, T]):

    async def get(self) -> T:
        return self._convert(await self.pv.get())


class PvMonitor(PvBase[ae.PvMonitor, T]):

    def get(self) -> T:
        return self._convert(self.pv.get())

    async def __aiter__(self) -> AsyncIterator[T]:
        async for value in self.pv:
            yield self._convert(value)


class PvArray(Pv[A], PvArrayBase[ae.Pv, A]):
    pass


class PvArrayMonitor(PvMonitor[A], PvArrayBase[ae.Pv, A]):
    pass


_int_names = ["char", "short", "int", "long", "enum"]
_float_names = ["float", "double"]


class _PvBoolBase(PvBase[P, bool]):

    def __init__(self, pv: P) -> None:
        assert pv.raw.type in _int_names
        assert pv.nelm == 1
        super().__init__(pv)

    @staticmethod
    def _convert(value: Any) -> bool:
        assert isinstance(value, int) and value in [0, 1]
        return bool(value)


class _PvIntBase(PvBase[P, int]):

    def __init__(self, pv: P) -> None:
        assert pv.raw.type in _int_names
        assert pv.nelm == 1
        super().__init__(pv)

    @staticmethod
    def _convert(value: Any) -> int:
        assert isinstance(value, int)
        return int(value)


class _PvFloatBase(PvBase[P, float]):

    def __init__(self, pv: P) -> None:
        assert pv.raw.type in _float_names
        assert pv.nelm == 1
        super().__init__(pv)

    @staticmethod
    def _convert(value: Any) -> float:
        assert isinstance(value, float)
        return float(value)


class _PvStrBase(PvBase[P, str]):

    def __init__(self, pv: P) -> None:
        assert pv.raw.type == "string"
        assert pv.nelm == 1
        super().__init__(pv)

    @staticmethod
    def _convert(value: Any) -> str:
        assert isinstance(value, str)
        return str(value)


class _PvArrayIntBase(PvArrayBase[P, NDArray[np.int32]]):

    def __init__(self, pv: P) -> None:
        assert pv.raw.type in _int_names
        assert pv.nelm > 1
        super().__init__(pv)

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


class _PvArrayFloatBase(PvArrayBase[P, NDArray[np.float64]]):

    def __init__(self, pv: P) -> None:
        assert pv.raw.type in _float_names
        assert pv.nelm > 1
        super().__init__(pv)

    @staticmethod
    def _convert(array: Any) -> np.ndarray[Any, np.dtype[np.float64]]:
        try:
            assert isinstance(array, np.ndarray)
        except AssertionError:
            assert isinstance(array, float)
            return np.array([array], dtype=np.float64)
        else:
            assert array.dtype == np.float64
            return array


class PvType(IntEnum):
    BOOL = 0
    INT = 1
    FLOAT = 2
    STR = 3
    ARRAY_INT = 4
    ARRAY_FLOAT = 5

    def base_type(self) -> Type[PvBase[P, T]]:
        types: List[Type[Any]] = [
            _PvBoolBase[P],
            _PvIntBase[P],
            _PvFloatBase[P],
            _PvStrBase[P],
            _PvArrayIntBase[P],
            _PvArrayFloatBase[P],
        ]
        return types[int(self)]


@dataclass
class Ca:
    "Channel Access context"

    timeout: Optional[float] = None

    @overload
    async def connect(self, name: str, type: Literal[PvType.BOOL], monitor: Literal[False] = False) -> Pv[bool]:
        ...

    @overload
    async def connect(self, name: str, type: Literal[PvType.INT], monitor: Literal[False] = False) -> Pv[int]:
        ...

    @overload
    async def connect(self, name: str, type: Literal[PvType.FLOAT], monitor: Literal[False] = False) -> Pv[float]:
        ...

    @overload
    async def connect(self, name: str, type: Literal[PvType.STR], monitor: Literal[False] = False) -> Pv[str]:
        ...

    @overload
    async def connect(
        self,
        name: str,
        type: Literal[PvType.ARRAY_INT],
        monitor: Literal[False] = False,
    ) -> PvArray[NDArray[np.int32]]:
        ...

    @overload
    async def connect(
        self,
        name: str,
        type: Literal[PvType.ARRAY_FLOAT],
        monitor: Literal[False] = False,
    ) -> PvArray[NDArray[np.float64]]:
        ...

    @overload
    async def connect(self, name: str, type: Literal[PvType.BOOL], monitor: Literal[True]) -> PvMonitor[bool]:
        ...

    @overload
    async def connect(self, name: str, type: Literal[PvType.INT], monitor: Literal[True]) -> PvMonitor[int]:
        ...

    @overload
    async def connect(self, name: str, type: Literal[PvType.FLOAT], monitor: Literal[True]) -> PvMonitor[float]:
        ...

    @overload
    async def connect(self, name: str, type: Literal[PvType.STR], monitor: Literal[True]) -> PvMonitor[str]:
        ...

    @overload
    async def connect(
        self,
        name: str,
        type: Literal[PvType.ARRAY_INT],
        monitor: Literal[True],
    ) -> PvArrayMonitor[NDArray[np.int32]]:
        ...

    @overload
    async def connect(
        self,
        name: str,
        type: Literal[PvType.ARRAY_FLOAT],
        monitor: Literal[True],
    ) -> PvArrayMonitor[NDArray[np.float64]]:
        ...

    async def connect(self, name: str, type: PvType, monitor: bool = False) -> PvBase[P, T]:
        Base: Type[PvBase[P, T]] = type.base_type()

        Kind: Type[PvBase[P, T]]
        apv: Awaitable[ae.PvBase]
        if not monitor:
            Kind = Pv[T] # type: ignore
            apv = ae.Pv.connect(name)
        else:
            Kind = PvMonitor[T] # type: ignore
            apv = ae.PvMonitor.connect(name)

        class SpecificPv(Base, Kind): # type: ignore
            pass

        try:
            pv = await asyncio.wait_for(apv, timeout=self.timeout)
        except asyncio.exceptions.TimeoutError:
            raise TimeoutError(f"Cannot connect to PV '{name}'")

        return SpecificPv(pv)
