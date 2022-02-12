from __future__ import annotations
from typing import Any, AsyncGenerator, Generic, List, Literal, TypeVar, overload, Union

from enum import Enum
from contextlib import asynccontextmanager

import caproto.asyncio.client as cac # type: ignore

T = TypeVar("T", bool, int, float, str, List[int], List[float])


class PvType(Enum):
    BOOL = 0,
    INT = 1,
    FLOAT = 2,
    STR = 3,
    ARRAY_INT = 4,
    ARRAY_FLOAT = 5,


class Context:

    def __init__(self) -> None:
        self._raw = cac.Context()

    @overload
    async def pv(self, name: str, pv_type: Literal[PvType.BOOL]) -> Pv[bool]:
        ...

    @overload
    async def pv(self, name: str, pv_type: Literal[PvType.INT]) -> Pv[int]:
        ...

    @overload
    async def pv(self, name: str, pv_type: Literal[PvType.FLOAT] = ...) -> Pv[float]:
        ...

    @overload
    async def pv(self, name: str, pv_type: Literal[PvType.STR]) -> Pv[str]:
        ...

    @overload
    async def pv(self, name: str, pv_type: Literal[PvType.ARRAY_INT]) -> Pv[List[int]]:
        ...

    @overload
    async def pv(self, name: str, pv_type: Literal[PvType.ARRAY_FLOAT]) -> Pv[List[float]]:
        ...

    async def pv(self, name: str, pv_type: PvType = PvType.FLOAT) -> _PvAny:
        # TODO: Change to `match` statement on migration to 3.10
        raw_pv = (await self._raw.get_pvs(name))[0]
        if pv_type == PvType.BOOL:
            return _PvBool(raw_pv)
        elif pv_type == PvType.INT:
            return _PvInt(raw_pv)
        elif pv_type == PvType.FLOAT:
            return _PvFloat(raw_pv)
        elif pv_type == PvType.STR:
            return _PvStr(raw_pv)
        elif pv_type == PvType.ARRAY_INT:
            return _PvArrayInt(raw_pv)
        elif pv_type == PvType.ARRAY_FLOAT:
            return _PvArrayFloat(raw_pv)
        else:
            raise RuntimeError("Unreachable")


class Pv(Generic[T]):

    Type: PvType

    def __init__(self, raw: cac.PV) -> None:
        self._raw = raw

    @staticmethod
    def _get_value(data: Any) -> T:
        raise NotImplementedError()

    async def write(self, value: T) -> None:
        await self._raw.write(value)

    async def read(self) -> T:
        return self._get_value((await self._raw.read()).data)

    async def _monitor(self, sub: cac.Subscription) -> AsyncGenerator[T, None]:
        async for res in sub:
            yield self._get_value(res.data)

    @asynccontextmanager
    async def monitor(self) -> AsyncGenerator[AsyncGenerator[T, None], None]:
        sub = self._raw.subscribe()
        try:
            yield self._monitor(sub)
        finally:
            await sub.clear()


_PvAny = Union[Pv[bool], Pv[int], Pv[float], Pv[str], Pv[List[int]], Pv[List[float]]]


class _PvBool(Pv[bool]):

    Type: PvType = PvType.BOOL

    @staticmethod
    def _get_value(data: Any) -> bool:
        return bool(data[0])


class _PvInt(Pv[int]):

    Type: PvType = PvType.INT

    @staticmethod
    def _get_value(data: Any) -> int:
        value = float(data[0])
        assert value.is_integer()
        return int(value)


class _PvFloat(Pv[float]):

    Type: PvType = PvType.FLOAT

    @staticmethod
    def _get_value(data: Any) -> float:
        return float(data[0])


class _PvStr(Pv[str]):

    Type: PvType = PvType.STR

    @staticmethod
    def _get_value(data: Any) -> str:
        return str(data[0])


class _PvArrayInt(Pv[List[int]]):

    Type: PvType = PvType.ARRAY_INT

    @staticmethod
    def _get_value(data: Any) -> List[int]:
        return [int(x) for x in data]


class _PvArrayFloat(Pv[List[float]]):

    Type: PvType = PvType.ARRAY_FLOAT

    @staticmethod
    def _get_value(data: Any) -> List[float]:
        return [float(x) for x in data]
