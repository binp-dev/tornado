from __future__ import annotations
from typing import Any, List, Optional

from dataclasses import dataclass
from subprocess import Popen, CalledProcessError
from pathlib import Path
from time import sleep

import logging

logger = logging.getLogger(__name__)


def make_ioc(ioc_dir: Path, arch: str, debug: bool = False) -> Ioc:
    return Ioc(
        ioc_dir / "bin" / arch / "PSC",
        ioc_dir / "iocBoot/iocPSC/st.cmd",
        debug=debug,
    )


@dataclass
class Ioc:
    binary: Path
    script: Path
    debug: bool = False
    proc: Optional[Popen[str]] = None
    args: Optional[List[str | Path]] = None

    def __enter__(self) -> Ioc:
        self.start()
        return self

    def __exit__(self, *args: Any) -> None:
        self.stop()

    def start(self) -> None:
        prefix = []
        if self.debug:
            prefix = ["gdb", "-batch", "-ex", "run", "-ex", "bt", "-args"]
        self.args = [*prefix, self.binary, self.script.name]
        self.proc = Popen(self.args, cwd=self.script.parent, text=True)
        logger.debug(f"IOC started: {self.args}")

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
