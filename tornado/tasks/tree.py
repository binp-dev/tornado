from __future__ import annotations
from typing import Dict

from pathlib import Path
from dataclasses import dataclass

from vortex.tasks.base import (
    task,
    TaskList,
    Context,
    Component,
    ComponentGroup,
    DictComponent,
)
from vortex.tasks.compiler import GccHost
from vortex.tasks.rust import RustcHost
from vortex.tasks.epics.epics_base import EpicsBaseHost, EpicsBaseCross

from tornado.tasks.toolchains import AppToolchain, AppRustc, McuToolchain, McuRustc
from tornado.tasks.app import AppReal, AppFake
from tornado.tasks.ioc import AppIocHost, AppIocCross
from tornado.tasks.fakedev import Fakedev
from tornado.tasks.freertos import Freertos
from tornado.tasks.mcu import Mcu


class HostTasks(ComponentGroup):
    def __init__(self) -> None:
        self.gcc = GccHost()
        self.rustc = RustcHost(self.gcc)
        self.epics_base = EpicsBaseHost(self.gcc)
        self.app = AppFake(self.rustc)
        self.ioc = AppIocHost(self.epics_base, self.app)
        self.fakedev = Fakedev(self.ioc, self.rustc)
        self.all = DictComponent(
            build=TaskList(
                self.epics_base.install,
                self.app.build,
                self.ioc.install,
                self.fakedev.build,
            ),
            test=TaskList(self.app.test, self.fakedev.test),
        )

    def components(self) -> Dict[str, Component]:
        return self.__dict__


class CrossTasks(ComponentGroup):
    def __init__(self, host: HostTasks) -> None:
        self.app_gcc = AppToolchain()
        self.app_rustc = AppRustc(self.app_gcc)
        self.mcu_gcc = McuToolchain()
        self.mcu_rustc = McuRustc(self.mcu_gcc)
        self.freertos = Freertos()
        self.epics_base = EpicsBaseCross(self.app_gcc, host.epics_base)
        self.app = AppReal(self.app_rustc)
        self.ioc = AppIocCross(self.epics_base, self.app)
        self.mcu = Mcu(self.mcu_gcc, self.mcu_rustc, self.freertos)

        build = TaskList(self.epics_base.install, self.ioc.install, self.mcu.build)
        deploy = TaskList(
            self.epics_base.deploy, self.ioc.deploy, self.mcu.deploy_and_reboot
        )

        @task
        def run(ctx: Context) -> None:
            deploy(ctx)
            self.ioc.run(ctx)

        self.all = DictComponent(build=build, deploy=deploy, run=run)

    def components(self) -> Dict[str, Component]:
        return self.__dict__


@dataclass
class AllTasks(ComponentGroup):
    def __init__(self) -> None:
        self.host = HostTasks()
        self.device = CrossTasks(self.host)

    def components(self) -> Dict[str, Component]:
        return self.__dict__
