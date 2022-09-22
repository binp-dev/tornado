from __future__ import annotations
from typing import Any, Dict, List

from pathlib import Path

from ferrite.utils.path import TargetPath
from ferrite.components.base import Context
from ferrite.components.rust import Rustc, RustcHost, RustcCross
from ferrite.components.app import AppBase

from tornado.components.ipp import Ipp
from tornado.info import path as self_path


class AbstractApp(AppBase):

    def __init__(
        self,
        rustc: Rustc,
        ipp: Ipp,
        features: List[str],
    ) -> None:
        super().__init__(
            self_path / "source/app",
            TargetPath("app"),
            rustc,
            deps=[ipp.generate_task],
            features=features,
        )
        self.ipp = ipp

    def env(self, ctx: Context) -> Dict[str, str]:
        return {
            **super().env(ctx),
            "TORNADO_IPP_DIR": str(ctx.target_path / self.ipp.output_dir),
        }


class AppFake(AbstractApp):

    def __init__(self, rustc: RustcHost, ipp: Ipp):
        super().__init__(rustc, ipp, features=["fake"])


class AppReal(AbstractApp):

    def __init__(self, rustc: RustcCross, ipp: Ipp):
        super().__init__(rustc, ipp, features=["real"])
