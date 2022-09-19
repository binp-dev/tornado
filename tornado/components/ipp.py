from __future__ import annotations

from pathlib import Path

from ferrite.components.protogen import ProtogenTest
from ferrite.components.rust import RustcHost
from ferrite.protogen.generator import Generator

from tornado.ipp import AppMsg, McuMsg


class Ipp(ProtogenTest):

    def __init__(
        self,
        ferrite_dir: Path,
        target_dir: Path,
        rustc: RustcHost,
    ):
        super().__init__(
            "ipp",
            ferrite_dir,
            target_dir / "ipp",
            Generator([AppMsg, McuMsg]),
            True,
            rustc,
        )
