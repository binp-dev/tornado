from __future__ import annotations
from typing import Any, Type, TypeVar, Generic

from dataclasses import dataclass
import asyncio
from asyncio import CancelledError, StreamReader, StreamWriter

from ferrite.protogen.base import UnexpectedEof, Value


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


M = TypeVar("M", bound=Value)


@dataclass
class MsgWriter(Generic[M]):
    Msg: Type[M]
    writer: StreamWriter

    async def write_msg(self, value: M) -> None:
        data = value.store()
        #print(f"- Stream write: {[int(b) for b in data]}")
        self.writer.write(data)
        await self.writer.drain()


@dataclass
class MsgReader(Generic[M]):
    Msg: Type[M]
    reader: StreamReader
    chunk_size: int
    buffer: bytes = b""

    async def read_msg(self) -> M:
        while True:
            try:
                #print(f"- Stream read: {[int(b) for b in self.buffer]}")
                msg = self.Msg.load(self.buffer)
                #print(f"- Msg size: {msg.size()}")
                self.buffer = self.buffer[msg.size():]
                return msg
            except UnexpectedEof as e:
                exc = e
                pass

            chunk = await self.reader.read(self.chunk_size)
            if len(chunk) == 0:
                raise exc
            self.buffer += chunk
