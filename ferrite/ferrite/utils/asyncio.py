from __future__ import annotations
from typing import Any, Awaitable, TypeVar

import asyncio
from asyncio import CancelledError, ensure_future

T = TypeVar("T")


async def forever() -> None:
    while True:
        await asyncio.sleep(1.0)


async def cancel_and_wait(awaitable: Awaitable[Any]) -> None:
    future = ensure_future(awaitable)
    if future.done() or future.cancelled():
        return
    future.cancel()
    try:
        await future
    except CancelledError:
        pass


async def with_background(
    fore: Awaitable[T],
    back: Awaitable[Any],
) -> T:
    fore_task = ensure_future(fore)
    back_task = ensure_future(back)
    try:
        await asyncio.wait([fore_task, back_task], return_when=asyncio.FIRST_COMPLETED)
        if fore_task.done():
            await cancel_and_wait(back_task)
            return fore_task.result()
        else:
            try:
                back_task.result()
            except:
                await cancel_and_wait(fore_task)
                raise
            return await fore_task
    except CancelledError:
        await cancel_and_wait(fore_task)
        await cancel_and_wait(back_task)
        raise
