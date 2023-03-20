from __future__ import annotations
from typing import Dict

from pathlib import Path
from dataclasses import dataclass

from vortex.utils.path import TargetPath
from vortex.tasks.base import task, Context, ComponentGroup
from vortex.tasks.compiler import GccHost
from vortex.tasks.rust import RustcHost
from vortex.tasks.epics.epics_base import EpicsBaseHost, EpicsBaseCross

from tornado.tasks.toolchains import AppGcc, AppRustc, McuGcc, McuRustc
from tornado.tasks.app.all import AppGroupHost, AppGroupCross
from tornado.tasks.fakedev import Fakedev
from tornado.tasks.freertos import Freertos
from tornado.tasks.mcu import McuGroup


class HostGroup(ComponentGroup):
    def __init__(self, path: Path) -> None:
        self.gcc = GccHost()
        self.rustc = RustcHost(self.gcc)
        self.epics_base = EpicsBaseHost(self.gcc)
        self.app = AppGroupHost(self.rustc, self.epics_base, path / "app", TargetPath("app"))
        self.fakedev = Fakedev(self.app.ioc, self.rustc, path / "test/fakedev", TargetPath("fakedev"))

    @task
    def build(self, ctx: Context) -> None:
        self.epics_base.install(ctx)
        self.app.build(ctx)
        self.app.ioc.install(ctx)
        self.fakedev.build(ctx)

    @task
    def test(self, ctx: Context) -> None:
        self.app.user.test(ctx)
        self.fakedev.test(ctx)


class CrossGroup(ComponentGroup):
    def __init__(self, host: HostGroup, path: Path) -> None:
        app_gcc = AppGcc()
        app_rustc = AppRustc(app_gcc)
        mcu_gcc = McuGcc()
        mcu_rustc = McuRustc(mcu_gcc)
        self.freertos = Freertos()
        self.epics_base = EpicsBaseCross(app_gcc, host.epics_base)
        self.app = AppGroupCross(app_rustc, self.epics_base, path / "app", TargetPath("app"))
        self.mcu = McuGroup(mcu_gcc, mcu_rustc, self.freertos, path / "mcu", TargetPath("mcu"))

    @task
    def build(self, ctx: Context) -> None:
        self.epics_base.install(ctx)
        self.app.ioc.install(ctx)
        self.mcu.build(ctx)

    @task
    def deploy(self, ctx: Context) -> None:
        self.epics_base.deploy(ctx)
        self.app.ioc.deploy(ctx)
        self.mcu.deploy_and_reboot(ctx)

    @task
    def run(self, ctx: Context) -> None:
        self.deploy(ctx)
        self.app.ioc.run(ctx)


@dataclass
class AllGroup(ComponentGroup):
    def __init__(self, path: Path) -> None:
        self.host = HostGroup(path)
        self.device = CrossGroup(self.host, path)
