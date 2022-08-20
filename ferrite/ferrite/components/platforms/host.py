from __future__ import annotations

from pathlib import Path

from ferrite.components.compiler import GccHost
from ferrite.components.rust import RustcHost


class HostAppPlatform:

    def __init__(self, target_dir: Path) -> None:
        self.gcc = GccHost()
        self.rustc = RustcHost(target_dir, self.gcc)
