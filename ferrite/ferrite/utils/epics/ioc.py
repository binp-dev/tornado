from __future__ import annotations
from typing import Any, Optional

import time
from subprocess import Popen
from pathlib import Path

import logging

logger = logging.getLogger(__name__)


def make_ioc(ioc_dir: Path, arch: str) -> Ioc:
    return Ioc(
        ioc_dir / "bin" / arch / "PSC",
        ioc_dir / "iocBoot/iocPSC/st.cmd",
    )


class Ioc:

    def __init__(self, binary: Path, script: Path) -> None:
        self.binary = binary
        self.script = script
        self.proc: Optional[Popen[str]] = None

    def __enter__(self) -> None:
        self.proc = Popen([self.binary, self.script.name], cwd=self.script.parent, text=True)
        time.sleep(1)
        logger.debug("ioc '%s' started")

    def __exit__(self, *args: Any) -> None:
        logger.debug("terminating '%s' ...")
        assert self.proc is not None
        self.proc.terminate()
        logger.debug("ioc '%s' terminated")
