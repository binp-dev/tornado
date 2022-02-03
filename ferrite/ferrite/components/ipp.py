from __future__ import annotations

from pathlib import Path

from ferrite.components.toolchain import Toolchain
from ferrite.components.codegen import Codegen


class Ipp(Codegen):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
    ):
        from ferrite.ipp import generate

        super().__init__(source_dir, target_dir, toolchain, "ipp", generate)
