from __future__ import annotations
from typing import Any, Dict, List

from pathlib import Path

from ferrite.components.rust import Rustc, RustcHost, RustcCross
from ferrite.components.app import AppBase

from tornado.components.ipp import Ipp


class AbstractApp(AppBase):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        rustc: Rustc,
        ipp: Ipp,
        features: List[str],
    ) -> None:
        super().__init__(
            source_dir / "app",
            target_dir / "app",
            rustc,
            deps=[ipp.generate_task],
            envs={"IPP_DIR": str(ipp.output_dir)},
            features=features,
        )
        self.ipp = ipp


class AppFake(AbstractApp):

    def __init__(self, source_dir: Path, target_dir: Path, rustc: RustcHost, ipp: Ipp):
        super().__init__(source_dir, target_dir, rustc, ipp, features=["fake"])


class AppReal(AbstractApp):

    def __init__(self, source_dir: Path, target_dir: Path, rustc: RustcCross, ipp: Ipp):
        super().__init__(source_dir, target_dir, rustc, ipp, features=["real"])
