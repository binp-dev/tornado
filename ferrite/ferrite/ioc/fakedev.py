from __future__ import annotations
from typing import List

from pathlib import Path
import asyncio
from asyncio import StreamReader, StreamWriter, Task, CancelledError

from ferrite.utils.epics.ioc import make_ioc
from time import sleep


class FakedevTest:

    def __init__(self) -> None:
        self.conn_task: Task[None] | None = None

    async def connect(self, reader: StreamReader, writer: StreamWriter) -> None:
        assert self.conn_task is None
        self.conn_task = asyncio.current_task()

        recv_msg = b"Hello, Fakedev!"
        recv_buf = await reader.read(len(recv_msg))
        print(f"F <- A: {recv_buf!r}")
        assert recv_buf == recv_msg

        send_msg = b"Hi, App!"
        writer.write(send_msg)
        await writer.drain()
        print(f"F -> A: {send_msg!r}")

    async def connect_once(self, reader: StreamReader, writer: StreamWriter) -> None:
        try:
            await self.connect(reader, writer)
        finally:
            self.server.close()

    async def run(self, app_bin: Path) -> None:
        self.server = await asyncio.start_server(self.connect_once, "127.0.0.1", 4884)

        process = await asyncio.create_subprocess_exec(app_bin)
        assert await process.wait() == 0

        await self.server.wait_closed()

        assert self.conn_task is not None
        await self.conn_task


def test(app_bin: Path) -> None:
    asyncio.run(FakedevTest().run(app_bin))


def run(ioc_dir: Path, arch: str) -> None:
    with make_ioc(ioc_dir, arch) as ioc:
        ioc.wait()
