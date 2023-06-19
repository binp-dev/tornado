from __future__ import annotations
from typing import List

from pathlib import Path

from vortex.utils.path import TargetPath
from vortex.tasks.rust import Rustc, RustcHost, RustcCross, Cargo


class AbstractApp(Cargo):
    def __init__(self, rustc: Rustc, src: Path, dst: TargetPath, features: List[str]) -> None:
        super().__init__(src, dst, rustc, features=features, default_features=False, release=True)
        self.lib_name = "libapp.so"
        self.lib_dir = self.bin_dir


class AppFake(AbstractApp):
    def __init__(self, rustc: RustcHost, src: Path, dst: TargetPath):
        super().__init__(rustc, src, dst, features=["fake"])


class AppReal(AbstractApp):
    def __init__(self, rustc: RustcCross, src: Path, dst: TargetPath):
        super().__init__(rustc, src, dst, features=["real"])
