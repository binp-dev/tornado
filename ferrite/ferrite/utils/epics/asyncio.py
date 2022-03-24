from __future__ import annotations
from typing import Any, AsyncGenerator, Generic, List, Literal, Type, TypeVar, AsyncContextManager, overload, Union

from enum import Enum
from dataclasses import dataclass

import asyncio
from asyncio import AbstractEventLoop, Future, Queue

from epics import PV # type: ignore

import logging

logger = logging.getLogger(__name__)

T = TypeVar("T", bool, int, float, str, List[int], List[float])


class _PvConnectFuture(Future[PV]):

    def _cancel(self) -> None:
        self._pv.disconnect()

    def _complete(self) -> None:
        self.set_result(self._pv)

    def _connection_callback(self, pvname: str = "", conn: bool = False, **kw: Any) -> None:
        assert pvname == self._name
        if conn:
            assert self.remove_done_callback(_PvConnectFuture._cancel) == 1
            loop = self.get_loop()
            if not loop.is_closed():
                loop.call_soon_threadsafe(self._complete)

    def __init__(self, name: str) -> None:
        super().__init__()

        self._name = name

        self.add_done_callback(_PvConnectFuture._cancel)
        self._pv = PV(
            name,
            form="native",
            auto_monitor=True,
            connection_callback=self._connection_callback,
        )


class _PvPutFuture(Future[None], Generic[T]):

    def _complete(self) -> None:
        if not self.done():
            self.set_result(None)

    def _callback(self, **kw: Any) -> None:
        loop = self.get_loop()
        if not loop.is_closed():
            loop.call_soon_threadsafe(self._complete)

    def __init__(self, pv: Pv[T], value: T) -> None:
        super().__init__()
        pv._raw.put(value, wait=False, callback=self._callback)


@dataclass
class _PvMonitorGenerator(AsyncGenerator[T, None]):
    _pv: Pv[T]
    _loop: AbstractEventLoop

    def __post_init__(self) -> None:
        self._queue: Queue[T | None] = Queue()
        self._done = False

    def _callback(self, value: Any = None, **kw: Any) -> None:
        if not self._loop.is_closed():
            self._loop.call_soon_threadsafe(lambda: self._queue.put_nowait(self._pv._convert(value)))

    def _cancel(self) -> None:
        self._done = True
        self._queue.put_nowait(None)

    def __aiter__(self) -> AsyncGenerator[T, None]:
        return self

    async def __anext__(self) -> T:
        value = await self._queue.get()
        if not self._done:
            assert value is not None
            return value
        else:
            assert value is None
            raise StopAsyncIteration()

    async def aclose(self) -> None:
        self._cancel()

    async def asend(self, value: Any) -> T:
        logger.warning("_PvMonitorGenerator.asend() called")
        return await self.__anext__()

    async def athrow(self, *args: Any, **kw: Any) -> T:
        logger.warning("_PvMonitorGenerator.athrow() called")
        return await self.__anext__()


@dataclass
class _PvMonitorContextManager(AsyncContextManager[_PvMonitorGenerator[T]]):
    _pv: Pv[T]
    _ret_cur: bool
    _gen: _PvMonitorGenerator[T] | None = None

    async def __aenter__(self) -> _PvMonitorGenerator[T]:
        assert self._gen is None
        self._gen = _PvMonitorGenerator(self._pv, asyncio.get_running_loop())
        self._pv._raw.add_callback(self._gen._callback)
        if self._ret_cur:
            self._pv._raw.run_callbacks()
        return self._gen

    async def __aexit__(self, *args: Any) -> None:
        assert self._gen is not None
        self._pv._raw.remove_callback(self._gen._callback)
        self._gen = None


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

    def _check_pv(self, raw: PV) -> None:
        int_names = ["char", "short", "int", "long", "enum"]
        float_names = ["float", "double"]

        try:
            if self == PvType.BOOL:
                assert raw.type in int_names
                assert raw.nelm == 1
            elif self == PvType.INT:
                assert raw.type in int_names
                assert raw.nelm == 1
            elif self == PvType.FLOAT:
                assert raw.type in float_names
                assert raw.nelm == 1
            elif self == PvType.STR:
                assert raw.type == "string"
                assert raw.nelm == 1
            elif self == PvType.ARRAY_INT:
                assert raw.type in int_names
                assert raw.nelm > 1
            elif self == PvType.ARRAY_FLOAT:
                assert raw.type in float_names
                assert raw.nelm > 1
            else:
                raise RuntimeError("Unreachable")
        except AssertionError:
            raise AssertionError(f"Actual PV type '{raw.type}' (nelm: {raw.nelm}) does not match requested type {self}")


class Pv(Generic[T]):
    type: PvType

    def __init__(self, raw: PV) -> None:
        self.type._check_pv(raw)
        self._raw = raw

    @staticmethod
    def _convert(value: Any) -> T:
        raise NotImplementedError()

    async def get(self) -> T:
        return self._convert(self._raw.get(use_monitor=True))

    def put(self, value: T) -> _PvPutFuture[T]:
        return _PvPutFuture(self, value)

    # NOTE: Current value is not provided by default, set `current` to `True` if you need it.
    def monitor(self, current: bool = False) -> _PvMonitorContextManager[T]:
        return _PvMonitorContextManager(self, current)

    @property
    def name(self) -> str:
        assert isinstance(self._raw.pvname, str)
        return self._raw.pvname


class PvArray(Pv[T]):

    @property
    def nelm(self) -> int:
        assert isinstance(self._raw.nelm, int)
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


class _PvArrayInt(PvArray[List[int]]):

    type: PvType = PvType.ARRAY_INT

    @staticmethod
    def _convert(array: Any) -> List[int]:
        return [int(x) for x in array]


class _PvArrayFloat(PvArray[List[float]]):

    type: PvType = PvType.ARRAY_FLOAT

    @staticmethod
    def _convert(array: Any) -> List[float]:
        return [float(x) for x in array]


_PvAny = Union[Pv[bool], Pv[int], Pv[float], Pv[str], PvArray[List[int]], PvArray[List[float]]]


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
    async def connect(self, name: str, pv_type: Literal[PvType.ARRAY_INT]) -> PvArray[List[int]]:
        ...

    @overload
    async def connect(self, name: str, pv_type: Literal[PvType.ARRAY_FLOAT]) -> PvArray[List[float]]:
        ...

    async def connect(self, name: str, pv_type: PvType) -> _PvAny:
        raw = await _PvConnectFuture(name)
        return pv_type._type()(raw)
