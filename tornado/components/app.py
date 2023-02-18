from __future__ import annotations
from typing import List

from ferrite.utils.path import TargetPath
from ferrite.components.rust import Rustc, RustcHost, RustcCross
from ferrite.components.app import AppBase

from tornado.info import path as self_path


class AbstractApp(AppBase):

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


class AppFake(AbstractApp):

    def __init__(self, rustc: RustcHost):
        super().__init__(rustc, features=["tcp"])


class AppReal(AbstractApp):

    def __init__(self, rustc: RustcCross):
        super().__init__(rustc, features=["rpmsg"])
