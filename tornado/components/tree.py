from __future__ import annotations
from typing import Dict

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Component, ComponentGroup, DictComponent, TaskList, TaskWrapper
from ferrite.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from ferrite.components.platforms.base import Platform
from ferrite.components.platforms.host import HostPlatform
from ferrite.components.platforms.imx8mn import Imx8mnPlatform

from tornado.components.ipp import Ipp
from tornado.components.app import AppReal, AppFake
from tornado.components.ioc import AppIocHost, AppIocCross
from tornado.components.config import Config
from tornado.components.fakedev import Fakedev
from tornado.components.mcu import Mcu


class HostComponents(ComponentGroup):

    def __init__(self, platform: HostPlatform) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.epics_base = EpicsBaseHost(self.gcc)
        self.config = Config()
        self.ipp = Ipp(self.rustc)
        self.app = AppFake(self.rustc, self.config, self.ipp)
        self.ioc = AppIocHost(self.epics_base, self.app)
        self.fakedev = Fakedev(self.ioc, self.ipp)
        self.all = DictComponent({
            "build": TaskList([self.epics_base.install_task, self.app.build_task, self.ioc.install_task]),
            "test": TaskList([self.ipp.test_task, self.app.test_task, self.fakedev.test_task]),
        })

    def components(self) -> Dict[str, Component]:
        return self.__dict__


class CrossComponents(ComponentGroup):

    def __init__(self, host: HostComponents, platform: Platform) -> None:
        self.app_gcc = platform.app.gcc
        self.app_rustc = platform.app.rustc
        self.mcu_gcc = platform.mcu.gcc
        self.mcu_rustc = platform.mcu.rustc
        self.freertos = platform.mcu.freertos
        self.epics_base = EpicsBaseCross(self.app_gcc, host.epics_base)
        self.app = AppReal(self.app_rustc, host.config, host.ipp)
        self.ioc = AppIocCross(self.epics_base, self.app)
        self.mcu = Mcu(self.mcu_gcc, self.mcu_rustc, self.freertos, platform.mcu.deployer, host.config, host.ipp)

        build_task = TaskList([self.epics_base.install_task, self.ioc.install_task, self.mcu.build_task])
        deploy_task = TaskList([self.epics_base.deploy_task, self.ioc.deploy_task, self.mcu.deploy_and_reboot_task])
        run_task = TaskWrapper(self.ioc.run_task, [deploy_task])
        self.all = DictComponent({"build": build_task, "deploy": deploy_task, "run": run_task})

    def components(self) -> Dict[str, Component]:
        return self.__dict__


@dataclass
class AllComponents(ComponentGroup):

    def __init__(self) -> None:
        self.host = HostComponents(HostPlatform())
        self.device = CrossComponents(self.host, Imx8mnPlatform())

    def components(self) -> Dict[str, Component]:
        return self.__dict__


def make_components() -> ComponentGroup:
    tree = AllComponents()
    tree._update_names()
    return tree
