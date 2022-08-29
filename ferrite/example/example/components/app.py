from __future__ import annotations

from pathlib import Path

from ferrite.components.rust import Rustc
from ferrite.components.app import AppBase

from example.components.protocol import Protocol


class App(AppBase):

    def __init__(self, source_dir: Path, target_dir: Path, rustc: Rustc, proto: Protocol) -> None:
        super().__init__(
            source_dir / "app",
            target_dir / "app",
            rustc,
            envs={"FER_PROTO_DIR": str(proto.output_dir)},
        )
        self.proto = proto
