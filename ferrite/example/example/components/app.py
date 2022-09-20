from __future__ import annotations
from typing import Dict

from ferrite.utils.path import TargetPath
from ferrite.components.base import Context
from ferrite.components.rust import Rustc
from ferrite.components.app import AppBase

from example.components.protocol import Protocol
from example.info import path as self_path


class App(AppBase):

    def __init__(self, rustc: Rustc, proto: Protocol) -> None:
        super().__init__(
            self_path / "source/app",
            TargetPath("example/app"),
            rustc,
            deps=[proto.generate_task],
        )
        self.proto = proto

    def env(self, ctx: Context) -> Dict[str, str]:
        return {
            **super().env(ctx),
            "FER_PROTO_DIR": str(ctx.target_path / self.proto.output_dir),
        }
