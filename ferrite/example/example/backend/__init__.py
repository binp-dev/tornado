from __future__ import annotations
from typing import Any, Callable, List

import os
from pathlib import Path
import asyncio
import numpy as np

from ferrite.utils.asyncio.net import TcpListener, MsgWriter, MsgReader
from ferrite.utils.epics.pv import Context, Pv, PvType
from ferrite.utils.epics.ioc import AsyncIoc
import ferrite.utils.epics.ca as ca

from example.protocol import InMsg, OutMsg


async def _async_test(epics_base_dir: Path, ioc_dir: Path, arch: str) -> None:
    async with TcpListener("127.0.0.1", 4884) as lis:
        async with AsyncIoc(epics_base_dir, ioc_dir, arch) as ioc:
            print("[backend] IOC started")
            async for stream in lis:
                break
            writer = MsgWriter(InMsg, stream.writer)
            reader = MsgReader(OutMsg, stream.reader, 260)
            print("[backend] Socket connected")

            ctx = Context()
            ai = await ctx.connect("ai", PvType.FLOAT)
            ao = await ctx.connect("ao", PvType.FLOAT)
            aai = await ctx.connect("aai", PvType.ARRAY_INT)
            aao = await ctx.connect("aao", PvType.ARRAY_INT)
            print("[backend] Pvs connected")

            async def test_ao(x: int) -> None:
                await ao.put(float(x))
                print(f"[backend] - Pv put: {x}")
                msg = (await reader.read_msg()).variant
                assert isinstance(msg, OutMsg.Ao)
                y = msg.value
                print(f"[backend] - Msg received: {y}")
                assert x == y

            print("[backend] Test Ao:")
            await test_ao(0x12345678)

            async def test_ai(x: int) -> None:

                async def send() -> None:
                    await asyncio.sleep(0.1)
                    await writer.write_msg(InMsg(InMsg.Ai(x)))
                    print(f"[backend] - Msg sent: {x}")

                async def check() -> None:
                    async with ai.monitor() as mon:
                        async for y in mon:
                            print(f"[backend] - Pv get: {y}")
                            assert x == int(y)
                            break

                assert int(await ai.get()) != x
                await asyncio.gather(send(), check())

            print("[backend] Test Ai:")
            await test_ai(0x789abcde)

            async def test_aao(gx: Callable[[int], List[int]]) -> None:

                ax = np.array(gx(aao.nelm), dtype=np.int32)
                await aao.put(ax)
                print(f"[backend] - Pv put:\n{ax}")
                msg = (await reader.read_msg()).variant
                assert isinstance(msg, OutMsg.Aao)
                ay = msg.value
                print(f"[backend] - Msg received: {ay}")
                assert all((x == y for x, y in zip(ax, ay)))

            print("[backend] Test Aao:")
            await test_aao(lambda n: [i * 0x1234 for i in range(n)])

            async def test_aai(gx: Callable[[int], List[int]]) -> None:
                ax = gx(aai.nelm)

                async def send() -> None:
                    await asyncio.sleep(0.1)
                    await writer.write_msg(InMsg(InMsg.Aai(ax)))
                    print(f"[backend] - Msg sent: {ax}")

                async def check() -> None:
                    async with aai.monitor() as mon:
                        async for ay in mon:
                            print(f"[backend] - Pv get: {ay}")
                            assert all((x == y for x, y in zip(ax, ay)))
                            break

                await asyncio.gather(send(), check())

            print("[backend] Test Aai:")
            await test_aai(lambda n: [i * 0x4321 for i in range(n, 0, -1)])

            ioc.stop()


def test(epics_base_dir: Path, ioc_dir: Path, arch: str) -> None:
    os.environ.update(ca.local_env())
    with ca.Repeater(epics_base_dir, arch):
        asyncio.run(_async_test(epics_base_dir, ioc_dir, arch))
