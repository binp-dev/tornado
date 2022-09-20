from __future__ import annotations

from pathlib import Path

from ferrite.components.compiler import GccHost
from ferrite.components.rust import RustcHost


class HostPlatform:

    def __init__(self) -> None:
        self.gcc = GccHost()
        self.rustc = RustcHost(self.gcc)
