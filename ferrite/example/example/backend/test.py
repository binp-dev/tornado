from __future__ import annotations
from typing import Any, Generic, Optional, Sequence, TypeVar

from dataclasses import dataclass
from asyncio import Queue

from numpy.typing import NDArray

from ferrite.utils.asyncio.net import MsgWriter
from ferrite.utils.epics.pv import Context as Ca

from example.protocol import InMsg, OutMsg

import logging

logger = logging.getLogger(__name__)

T = TypeVar("T")


def assert_eq(a: T, b: T) -> None:
    assert a == b, f"{a} != {b}"


def assert_array_eq(a: NDArray[Any], b: NDArray[Any]) -> None:
    assert (a == b).all(), f"Arrays differ:\n{a}\n{b}"


@dataclass
class TestCase:
    ca: Ca

    def __post_init__(self) -> None:
        self.name = self.__class__.__name__

    async def run(self) -> None:
        raise NotImplementedError()

    async def run_with_log(self) -> None:
        try:
            await self.run()
        except RuntimeError:
            logger.info(f"FAILED '{self.name}'")
            raise
        else:
            logger.info(f"Ok '{self.name}'")


W = TypeVar("W", bound=InMsg.Variant)


@dataclass
class WriteTestCase(TestCase, Generic[W]):
    channel: MsgWriter[InMsg]

    async def write_msg(self, msg: W) -> None:
        await self.channel.write_msg(InMsg(msg))


R = TypeVar("R", bound=OutMsg.Variant)


class ReadTestCase(TestCase, Generic[R]):

    def __post_init__(self) -> None:
        super().__post_init__()
        self.queue: Queue[R] = Queue()

    def _take_msg(self, msg: OutMsg) -> Optional[R]:
        raise NotImplementedError()

    def _dispatch(self, msg: OutMsg) -> bool:
        m = self._take_msg(msg)
        if m is not None:
            self.queue.put_nowait(m)
            return True
        else:
            return False

    async def read_msg(self) -> R:
        return await self.queue.get()


def dispatch(tests: Sequence[TestCase], msg: OutMsg) -> None:
    for test in tests:
        if isinstance(test, ReadTestCase):
            if test._dispatch(msg):
                return
    raise AssertionError("Message isn't accepted by any test")
