from __future__ import annotations
from typing import Any

from dataclasses import dataclass
import asyncio
from asyncio import StreamReader, StreamWriter


@dataclass
class TcpStream:
    reader: StreamReader
    writer: StreamWriter


@dataclass
class TcpListener:
    host: str
    port: int

    async def _connect(self, reader: StreamReader, writer: StreamWriter) -> None:
        await self.queue.put(TcpStream(reader, writer))

    async def __aenter__(self) -> TcpListener:
        self.queue: asyncio.Queue[TcpStream] = asyncio.Queue()
        self.server = await asyncio.start_server(self._connect, "127.0.0.1", 4884)
        return self

    async def __aexit__(self, *args: Any) -> None:
        self.server.close()
        await self.server.wait_closed()

    def __aiter__(self) -> TcpListener:
        return self

    async def __anext__(self) -> TcpStream:
        return await self.queue.get()
