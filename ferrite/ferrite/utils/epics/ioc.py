from __future__ import annotations
from typing import Any, Optional

from dataclasses import dataclass
from subprocess import Popen
from pathlib import Path

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

    def __enter__(self) -> None:
        prefix = []
        if self.debug:
            prefix = ["gdb", "-batch", "-ex", "run", "-ex", "bt", "-args"]
        self.proc = Popen([*prefix, self.binary, self.script.name], cwd=self.script.parent, text=True)
        logger.debug("ioc '%s' started")

    def __exit__(self, *args: Any) -> None:
        logger.debug("terminating '%s' ...")
        assert self.proc is not None
        self.proc.terminate()
        logger.debug("ioc '%s' terminated")
