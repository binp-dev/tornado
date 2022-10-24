from __future__ import annotations
from typing import Any, Dict, List

from pathlib import Path

from ferrite.utils.path import TargetPath
from ferrite.components.base import Context
from ferrite.components.rust import Rustc, RustcHost, RustcCross
from ferrite.components.app import AppBase

from tornado.components.config import Config
from tornado.components.ipp import Ipp
from tornado.info import path as self_path


class AbstractApp(AppBase):

    def __init__(
        self,
        rustc: Rustc,
        config: Config,
        ipp: Ipp,
        features: List[str],
    ) -> None:
        super().__init__(
            self_path / "source/app",
            TargetPath("tornado/app"),
            rustc,
            deps=[config.generate_task, ipp.generate_task],
            features=features,
        )
        self.config = config
        self.ipp = ipp


class AppFake(AbstractApp):

    def __init__(self, rustc: RustcHost, config: Config, ipp: Ipp):
        super().__init__(rustc, config, ipp, features=["tcp"])


class AppReal(AbstractApp):

    def __init__(self, rustc: RustcCross, config: Config, ipp: Ipp):
        super().__init__(rustc, config, ipp, features=["rpmsg"])
