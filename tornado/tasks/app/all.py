from __future__ import annotations
from typing import Dict

from pathlib import Path

from vortex.utils.path import TargetPath
from vortex.tasks.base import task, Context, ComponentGroup
from vortex.tasks.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from vortex.tasks.compiler import Gcc, HOST_GCC
from vortex.tasks.rust import RustcHost, RustcCross
from vortex.tasks.cmake import Cmake

from .ioc import AbstractAppIoc, AppIocHost, AppIocCross
from .user import AbstractApp, AppReal, AppFake
from .plugin import Plugin


class AppGroupHost(ComponentGroup):
    def __init__(self, rustc: RustcHost, epics_base: EpicsBaseHost, src: Path, dst: TargetPath) -> None:
        self.user = AppFake(rustc, src / "user", dst / "user")
        self.plugin = Plugin(src / "plugin", dst / "plugin", HOST_GCC)
        self.ioc = AppIocHost(src / "ioc", dst / "ioc", epics_base, dylibs=[self.user, self.plugin])

    @task
    def build(self, ctx: Context) -> None:
        self.ioc.build(ctx)

    @task
    def run(self, ctx: Context) -> None:
        self.ioc.run(ctx)


class AppGroupCross(ComponentGroup):
    def __init__(self, rustc: RustcCross, epics_base: EpicsBaseCross, src: Path, dst: TargetPath) -> None:
        assert rustc.cc is epics_base.cc
        self.cc = rustc.cc
        self.rustc = rustc
        self.user = AppReal(self.rustc, src / "user", dst / "user")
        self.plugin = Plugin(src / "plugin", dst / "plugin", self.cc)
        self.ioc = AppIocCross(src / "ioc", dst / "ioc", epics_base, dylibs=[self.user, self.plugin])

    @task
    def build(self, ctx: Context) -> None:
        self.ioc.build(ctx)

    @task
    def deploy(self, ctx: Context) -> None:
        self.ioc.deploy(ctx)
