from __future__ import annotations
from typing import Any, Dict, List, Optional

import os
from subprocess import Popen, CalledProcessError
from pathlib import Path
from time import sleep
import asyncio

import logging

logger = logging.getLogger(__name__)


class IocBase:

    def __init__(self, epics_base_dir: Path, ioc_dir: Path, arch: str, debug: bool = False):
        self.binary = ioc_dir / "bin" / arch / "Fer"
        self.script = ioc_dir / "iocBoot/iocFer/st.cmd"
        self.lib_dirs = [epics_base_dir / "lib" / arch, ioc_dir / "lib" / arch]
        self.debug = debug

    def _cmd(self) -> List[str]:
        return [
            *(["gdb", "-batch", "-ex", "run", "-ex", "bt", "-args"] if self.debug else []),
            str(self.binary),
            self.script.name,
        ]

    def _cwd(self) -> Path:
        return self.script.parent

    def _env(self) -> Dict[str, str]:
        return {
            **dict(os.environ),
            "LD_LIBRARY_PATH": ":".join([str(p) for p in self.lib_dirs]),
        }


class Ioc(IocBase):

    def __enter__(self) -> Ioc:
        self.start()
        return self

    def __exit__(self, *args: Any) -> None:
        self.stop()

    def start(self) -> None:
        cmd = self._cmd()
        self.proc: Optional[Popen[str] | None] = Popen(cmd, cwd=self._cwd(), env=self._env(), text=True)
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
        cmd = self._cmd()
        self.stopped = False
        self.proc = await asyncio.create_subprocess_exec(*cmd, cwd=self._cwd(), env=self._env())
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
