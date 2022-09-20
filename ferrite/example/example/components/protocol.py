from __future__ import annotations

from pathlib import Path

from ferrite.components.protogen import ProtogenTest
from ferrite.components.rust import RustcHost
from ferrite.protogen.generator import Generator

from example.protocol import InMsg, OutMsg


class Protocol(ProtogenTest):

    def __init__(
        self,
        ferrite_dir: Path,
        target_dir: Path,
        rustc: RustcHost,
    ):
        super().__init__(
            "protocol",
            ferrite_dir,
            target_dir / "example/protocol",
            Generator([InMsg, OutMsg]),
            True,
            rustc,
        )
