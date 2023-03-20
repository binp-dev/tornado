from __future__ import annotations

from pathlib import Path

from ferrite.components.toolchain import HostToolchain
from ferrite.components.codegen import CodegenWithTest


class Ipp(CodegenWithTest):

    def __init__(
        self,
        source_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
    ):
        from tornado.ipp import generate

        super().__init__(
            source_dir / "ipp",
            ferrite_source_dir,
            target_dir,
            toolchain,
            "ipp",
            generate,
        )
