from __future__ import annotations
from typing import Any, TypeVar

import os
from pathlib import Path
import asyncio

import numpy as np
from numpy.typing import NDArray, DTypeLike

from ferrite.utils.asyncio.net import TcpListener, MsgWriter, MsgReader
from ferrite.utils.epics.pv import Context, Pv, PvType
from ferrite.utils.epics.ioc import AsyncIoc
import ferrite.utils.epics.ca as ca

from example.protocol import InMsg, OutMsg

import logging

logger = logging.getLogger(__name__)

T = TypeVar("T")


def assert_eq(a: T, b: T) -> None:
    assert a == b, f"{a} != {b}"


def assert_array_eq(a: NDArray[Any], b: NDArray[Any]) -> None:
    assert (a == b).all(), f"Arrays differ:\n{a}\n{b}"


async def _ai_test(ctx: Context, channel: MsgWriter[InMsg]) -> None:
    logger.info("Test Ai")
    record = await ctx.connect("ai", PvType.FLOAT)

    async def test(x: int) -> None:
        assert int(await record.get()) != x

        async with record.monitor() as mon:
            await channel.write_msg(InMsg(InMsg.Ai(x)))
            logger.debug(f"Msg sent: {x}")

            async for y in mon:
                logger.debug(f"Pv get: {y}")
                assert_eq(x, int(y))
                break

    await test(0x789abcde)


async def _ao_test(ctx: Context, channel: MsgReader[OutMsg]) -> None:
    logger.info("Test Ao")
    record = await ctx.connect("ao", PvType.FLOAT)

    async def test(x: int) -> None:
        await record.put(float(x))
        logger.debug(f"Pv put: {x}")

        msg = (await channel.read_msg()).variant
        assert isinstance(msg, OutMsg.Ao)
        y = msg.value
        logger.debug(f"Msg received: {y}")

        assert_eq(x, y)

    await test(0x12345678)


async def _bi_test(ctx: Context, channel: MsgWriter[InMsg]) -> None:
    logger.info("Test Bi")
    record = await ctx.connect("bi", PvType.INT)

    async def test(x: bool) -> None:
        assert int(await record.get()) != x

        async with record.monitor() as mon:
            await channel.write_msg(InMsg(InMsg.Bi(int(x))))
            logger.debug(f"Msg sent: {int(x)}")

            async for y in mon:
                logger.debug(f"Pv get: {y}")
                assert_eq(int(x), y)
                break

    await test(True)
    await test(False)


async def _bo_test(ctx: Context, channel: MsgReader[OutMsg]) -> None:
    logger.info("Test Bo")
    record = await ctx.connect("bo", PvType.INT)

    async def test(x: bool) -> None:
        await record.put(int(x))
        logger.debug(f"Pv put: {int(x)}")

        msg = (await channel.read_msg()).variant
        assert isinstance(msg, OutMsg.Bo)
        y = msg.value
        logger.debug(f"Msg received: {y}")

        assert_eq(int(x), y)

    await test(True)
    await test(False)


async def _aai_test(ctx: Context, channel: MsgWriter[InMsg]) -> None:
    logger.info("Test Aai")
    record = await ctx.connect("aai", PvType.ARRAY_INT)

    async def test(ax: NDArray[np.int32]) -> None:
        async with record.monitor() as mon:
            await channel.write_msg(InMsg(InMsg.Aai(ax)))
            logger.debug(f"Msg sent:\n{ax}")

            async for ay in mon:
                logger.debug(f"Pv get:\n{ay}")
                assert_array_eq(ax, ay)
                break

    await test(np.arange(record.nelm, 0, -1, dtype=np.int32) * 0x1234)


async def _aao_test(ctx: Context, channel: MsgReader[OutMsg]) -> None:
    logger.info("Test Aao")
    record = await ctx.connect("aao", PvType.ARRAY_INT)

    async def test(ax: NDArray[np.int32]) -> None:
        await record.put(ax)
        logger.debug(f"Pv put:\n{ax}")

        msg = (await channel.read_msg()).variant
        assert isinstance(msg, OutMsg.Aao)
        ay = msg.values
        logger.debug(f"Msg received:\n{ay}")

        assert_array_eq(ax, ay)

    await test(np.arange(record.nelm, dtype=np.int32) * 0x4321)


async def _waveform_test(ctx: Context, channel: MsgWriter[InMsg]) -> None:
    logger.info("Test Waveform")
    record = await ctx.connect("waveform", PvType.ARRAY_INT)

    async def test(ax: NDArray[np.int32]) -> None:
        async with record.monitor() as mon:
            await channel.write_msg(InMsg(InMsg.Waveform(ax)))
            logger.debug(f"Msg sent:\n{ax}")

            async for ay in mon:
                logger.debug(f"Pv get:\n{ay}")
                assert_array_eq(ax, ay)
                break

    await test(np.arange(record.nelm, dtype=np.int32) * -0x1234)


async def _mbbi_direct_test(ctx: Context, channel: MsgWriter[InMsg]) -> None:
    logger.info("Test MbbiDirect")

    nbits = 16
    records = list(await asyncio.gather(*[ctx.connect(f"mbbiDirect.B{i:X}", PvType.INT) for i in range(nbits)]))

    async def test(x: int, i: int, b: bool) -> int:
        x = x | (1 << i) if b else x & ~(1 << i)

        async with records[i].monitor() as mon:
            await channel.write_msg(InMsg(InMsg.MbbiDirect(x)))
            logger.debug(f"Msg sent: {x}")

            async for y in mon:
                logger.debug(f"Pv[{i}] get: {y}")
                assert_eq(int(b), y)
                break

            return x

    x = 0
    for i in range(nbits):
        x = await test(x, i, True)
    for i in range(nbits):
        x = await test(x, i, False)


async def _mbbo_direct_test(ctx: Context, channel: MsgReader[OutMsg]) -> None:
    logger.info("Test MbboDirect")

    nbits = 16
    records = list(await asyncio.gather(*[ctx.connect(f"mbboDirect.B{i:X}", PvType.INT) for i in range(nbits)]))

    async def test(x: int, i: int, b: bool) -> int:
        await records[i].put(int(b))
        logger.debug(f"Pv[{i}] put: {int(b)}")
        x = x | (1 << i) if b else x & ~(1 << i)

        msg = (await channel.read_msg()).variant
        assert isinstance(msg, OutMsg.MbboDirect)
        y = msg.value
        logger.debug(f"Msg received: {y}")
        assert_eq(x, y)

        return x

    x = 0
    for i in range(nbits):
        x = await test(x, i, True)
    for i in range(nbits):
        x = await test(x, i, False)


async def _async_test(epics_base_dir: Path, ioc_dir: Path, arch: str) -> None:
    async with TcpListener("127.0.0.1", 4884) as lis:
        async with AsyncIoc(epics_base_dir, ioc_dir, arch) as ioc:
            logger.info("IOC started")
            async for stream in lis:
                break
            writer = MsgWriter(InMsg, stream.writer)
            reader = MsgReader(OutMsg, stream.reader, 260)
            logger.info("Socket connected")

            ctx = Context()
            await _ai_test(ctx, writer),
            await _ao_test(ctx, reader),
            await _bi_test(ctx, writer),
            await _bo_test(ctx, reader),
            await _aai_test(ctx, writer),
            await _aao_test(ctx, reader),
            await _waveform_test(ctx, writer),
            await _mbbi_direct_test(ctx, writer),
            await _mbbo_direct_test(ctx, reader),

            ioc.stop()


def test(epics_base_dir: Path, ioc_dir: Path, arch: str) -> None:
    os.environ.update(ca.local_env())
    with ca.Repeater(epics_base_dir, arch):
        asyncio.run(_async_test(epics_base_dir, ioc_dir, arch))
