from __future__ import annotations
from typing import List

import asyncio


class TestServer:

    def __init__(self, host: str, port: int) -> None:
        self.loop = asyncio.new_event_loop()
        self.test_task: asyncio.Task[None] | None = None
        self.server = self.loop.run_until_complete(asyncio.start_server(self._test, host, port))

    async def _test(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        assert self.test_task is None
        self.test_task = asyncio.current_task()

        recv_msg = b"Hello, Fakedev!"
        recv_buf = await reader.read(len(recv_msg))
        print(f"F <- A: {recv_buf!r}")
        assert recv_buf == recv_msg

        send_msg = b"Hi, App!"
        writer.write(send_msg)
        await writer.drain()
        print(f"F -> A: {send_msg!r}")

        self.server.close()

    def wait(self) -> None:
        try:
            self.loop.run_until_complete(self.server.serve_forever())
        except asyncio.CancelledError:
            pass
        else:
            raise RuntimeError("asyncio.CancelledError hasn't been caught")

        assert self.test_task is not None
        self.loop.run_until_complete(self.test_task)


def test() -> None:
    TestServer("localhost", 4884).wait()
