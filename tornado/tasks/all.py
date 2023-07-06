from __future__ import annotations

from pathlib import Path
from dataclasses import dataclass

from vortex.utils.path import TargetPath
from vortex.dst.base import Device
from vortex.tasks.base import task, Context, ComponentGroup
from vortex.tasks.rust import RustcHost
from vortex.tasks.epics.epics_base import EpicsSource, EpicsRepo, EpicsBaseHost, EpicsBaseCross

from tornado.tasks.toolchains import AppGcc, AppRustc, McuGcc, McuRustc
from tornado.tasks.app.all import AppGroupHost, AppGroupCross
from tornado.tasks.fakedev import Fakedev
from tornado.tasks.freertos import Freertos
from tornado.tasks.mcu import McuGroup


class HostGroup(ComponentGroup):
    def __init__(self, path: Path, epics_src: EpicsSource) -> None:
        self.rustc = RustcHost()
        self.epics_base = EpicsBaseHost(epics_src, epics_src.prefix)
        self.app = AppGroupHost(self.rustc, self.epics_base, path / "app", TargetPath("app"))
        self.fakedev = Fakedev(self.app.ioc, self.rustc, path / "test/fakedev", TargetPath("fakedev"))

    @task
    def build(self, ctx: Context) -> None:
        self.epics_base.build(ctx)
        self.app.build(ctx)
        self.app.ioc.build(ctx)
        self.fakedev.build(ctx)

    @task
    def test(self, ctx: Context) -> None:
        self.fakedev.test(ctx)


class CrossGroup(ComponentGroup):
    def __init__(self, path: Path, epics_src: EpicsRepo) -> None:
        app_gcc = AppGcc()
        app_rustc = AppRustc(app_gcc)
        mcu_gcc = McuGcc()
        mcu_rustc = McuRustc(mcu_gcc)
        self.freertos = Freertos()
        self.epics_base = EpicsBaseCross(epics_src, epics_src.prefix, app_gcc)
        self.app = AppGroupCross(app_rustc, self.epics_base, path / "app", TargetPath("app"))
        self.mcu = McuGroup(mcu_gcc, mcu_rustc, self.freertos, path / "mcu", TargetPath("mcu"))

    @task
    def build(self, ctx: Context) -> None:
        self.epics_base.build(ctx)
        self.app.ioc.build(ctx)
        self.mcu.build(ctx)

    @task
    def deploy(self, ctx: Context) -> None:
        self.epics_base.deploy(ctx)
        self.app.ioc.deploy(ctx)
        self.mcu.deploy(ctx)

    @task
    def reboot(self, ctx: Context) -> None:
        assert isinstance(ctx.dst, Device)
        ctx.dst.reboot()


@dataclass
class AllGroup(ComponentGroup):
    def __init__(self, path: Path) -> None:
        epics_src = EpicsRepo("7.0.7", TargetPath("epics_base"))
        self.host = HostGroup(path, epics_src)
        self.device = CrossGroup(path, epics_src)
