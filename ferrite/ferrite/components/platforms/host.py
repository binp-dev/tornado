from __future__ import annotations

from pathlib import Path

from ferrite.components.toolchain import HostToolchain
from ferrite.components.rust import HostRustup


class HostAppPlatform:

    def __init__(self, target_dir: Path) -> None:
        self.gcc = HostToolchain()
        self.rustup = HostRustup(target_dir, self.gcc)
