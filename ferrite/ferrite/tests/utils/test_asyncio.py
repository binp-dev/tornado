from __future__ import annotations

from dataclasses import dataclass

import asyncio
from asyncio import CancelledError, ensure_future

from ferrite.utils.asyncio import forever, cancel_and_wait, with_background


async def _immediate() -> None:
    pass


@dataclass
class _Status:
    started: bool = False
    cancelled: bool = False
    finalized: bool = False


async def _observable(status: _Status) -> None:
    status.started = True
    try:
        await forever()
    except CancelledError:
        status.cancelled = True
        raise
    finally:
        status.finalized = True


async def test_wait_cancel() -> None:
    status = _Status()
    coro = _observable(status)
    assert not status.started

    task = ensure_future(coro)
    await asyncio.sleep(0.01)
    assert status.started and not status.cancelled and not status.finalized

    done, pending = await asyncio.wait([task], timeout=0.01)
    assert len(done) == 0 and len(pending) == 1
    assert status.started and not status.cancelled and not status.finalized

    await cancel_and_wait(task)
    assert status.cancelled and status.finalized


async def test_with_background_normal_stop_both() -> None:
    fore = ensure_future(_immediate())
    back = ensure_future(_immediate())

    await with_background(fore, back)
    assert fore.done() and back.done()


async def test_with_background_normal_stop_secondary() -> None:
    status = _Status()
    fore = ensure_future(_immediate())
    back = ensure_future(_observable(status))

    await with_background(fore, back)
    assert fore.done() and back.done()
    assert status.cancelled and status.finalized


class _TestException(RuntimeError):

    def __init__(self) -> None:
        super().__init__("Test")


async def _timeout_and_raise(timeout: float) -> None:
    await asyncio.sleep(timeout)
    raise _TestException()


async def test_with_background_primary_exception() -> None:
    status = _Status()

    fore = ensure_future(_timeout_and_raise(0.1))
    back = ensure_future(_observable(status))

    try:
        await with_background(fore, back)
    except _TestException as e:
        pass
    else:
        assert False, "Exception is expected"

    assert fore.done() and back.done()
    assert status.cancelled and status.finalized


async def test_with_background_secondary_exception() -> None:
    status = _Status()

    fore = ensure_future(_observable(status))
    back = ensure_future(_timeout_and_raise(0.1))

    try:
        await with_background(fore, back)
    except _TestException as e:
        pass
    else:
        assert False, "Exception is expected"

    assert fore.done()
    assert back.done()
    assert status.cancelled and status.finalized
