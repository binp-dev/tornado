from __future__ import annotations

from pathlib import Path

from ferrite.components.codegen import CodegenTest
from ferrite.components.rust import RustcHost
from ferrite.codegen.generator import Generator

from example.protocol import InMsg, OutMsg


class Protocol(CodegenTest):

    def __init__(
        self,
        ferrite_dir: Path,
        target_dir: Path,
        rustc: RustcHost,
    ):
        super().__init__(
            "protocol",
            ferrite_dir,
            target_dir / "protocol",
            Generator([InMsg, OutMsg]),
            rustc,
        )
