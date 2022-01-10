from __future__ import annotations
from typing import Any, Optional

import os
import time
from subprocess import Popen


class Ioc:

    def __init__(self, binary: str, script: str) -> None:
        self.binary = binary
        self.script = script
        self.proc: Optional[Popen[str]] = None

    def __enter__(self) -> None:
        self.proc = Popen([self.binary, os.path.basename(self.script)], cwd=os.path.dirname(self.script), text=True)
        time.sleep(1)
        print("ioc '%s' started")

    def __exit__(self, *args: Any) -> None:
        print("terminating '%s' ...")
        assert self.proc is not None
        self.proc.terminate()
        print("ioc '%s' terminated")
