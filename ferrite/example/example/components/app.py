from __future__ import annotations

from pathlib import Path

from ferrite.components.rust import Rustc
from ferrite.components.app import AppBase


class App(AppBase):

    def __init__(self, source_dir: Path, target_dir: Path, rustc: Rustc) -> None:
        super().__init__(source_dir / "app", target_dir / "app", rustc)
