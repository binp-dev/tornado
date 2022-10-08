from __future__ import annotations
from typing import Any, Dict, List, Optional

import os
from subprocess import Popen, CalledProcessError
from pathlib import Path
from time import sleep
from enum import Enum
import asyncio

import logging

logger = logging.getLogger(__name__)


class RunMode(Enum):
    NORMAL = 0
    DEBUGGER = 1
    PROFILER = 2


class IocBase:

    def __init__(
        self,
        epics_base_dir: Path,
        ioc_dir: Path,
        arch: str,
        env: Dict[str, str] = {},
        mode: RunMode = RunMode.NORMAL,
    ):
        self.binary = ioc_dir / "bin" / arch / "Fer"
        self.script = ioc_dir / "iocBoot/iocFer/st.cmd"
        self.lib_dirs = [epics_base_dir / "lib" / arch, ioc_dir / "lib" / arch]
        self._env = env
        self.mode = mode

    def cmd(self) -> List[str]:
        prefix = []
        if self.mode == RunMode.DEBUGGER:
            prefix = ["gdb", "-batch", "-ex", "run", "-ex", "bt", "-args"]
        elif self.mode == RunMode.PROFILER:
            prefix = ["perf", "record"]
        return [
            *prefix,
            str(self.binary),
            self.script.name,
        ]

    def cwd(self) -> Path:
        return self.script.parent

    def env(self) -> Dict[str, str]:
        return {
            **dict(os.environ),
            **self._env,
            "LD_LIBRARY_PATH": ":".join([str(p) for p in self.lib_dirs]),
        }


class Ioc(IocBase):

    def __enter__(self) -> Ioc:
        self.start()
        return self

    def __exit__(self, *args: Any) -> None:
        self.stop()

    def start(self) -> None:
        cmd = self.cmd()
        self.proc: Optional[Popen[str] | None] = Popen(cmd, cwd=self.cwd(), env=self.env(), text=True)
        logger.debug(f"IOC started: {cmd}")

    def stop(self) -> None:
        if self.proc is not None:
            logger.debug("Terminating IOC ...")
            self.proc.terminate()
            logger.debug(f"IOC terminated")
            self.proc = None
        else:
            logger.debug("IOC is already stopped")

    def wait(self) -> None:
        assert self.proc is not None
        while True:
            sleep(0.1)
            ret = self.proc.poll()
            if ret is not None:
                self.proc = None
                if ret != 0:
                    raise CalledProcessError(ret, [self.binary, self.script])
                break


class AsyncIoc(IocBase):

    async def __aenter__(self) -> AsyncIoc:
        cmd = self.cmd()
        self.stopped = False
        self.proc = await asyncio.create_subprocess_exec(*cmd, cwd=self.cwd(), env=self.env())
        await asyncio.sleep(1.0)
        logger.debug(f"IOC started: {cmd}")
        return self

    async def __aexit__(self, *args: Any) -> None:
        self.stop()
        await asyncio.sleep(0.1)

    async def wait(self) -> None:
        retcode = await self.proc.wait()
        if not self.stopped:
            assert retcode == 0

    def stop(self) -> None:
        self.stopped = True
        self.proc.terminate()
