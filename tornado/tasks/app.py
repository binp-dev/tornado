from __future__ import annotations
from typing import List

from vortex.utils.path import TargetPath
from vortex.tasks.rust import Rustc, RustcHost, RustcCross, Cargo

from tornado.manage.info import path as self_path


class AbstractApp(Cargo):
    def __init__(
        self,
        rustc: Rustc,
        features: List[str],
    ) -> None:
        super().__init__(
            self_path / "source/app",
            TargetPath("tornado/app"),
            rustc,
            features=features,
            default_features=False,
            release=True,
        )
        self.lib_name = "libapp.so"
        self.lib_path = self.bin_dir / self.lib_name


class AppFake(AbstractApp):
    def __init__(self, rustc: RustcHost):
        super().__init__(rustc, features=["fake"])


class AppReal(AbstractApp):
    def __init__(self, rustc: RustcCross):
        super().__init__(rustc, features=["real"])
