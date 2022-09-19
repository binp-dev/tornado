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
from tornado.components.fakedev import Fakedev
from tornado.components.mcu import Mcu


class HostComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        ferrite_dir: Path,
        target_dir: Path,
        platform: HostPlatform,
    ) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.epics_base = EpicsBaseHost(target_dir, self.gcc)
        self.ipp = Ipp(ferrite_dir, target_dir, self.rustc)
        self.app = AppFake(source_dir, target_dir, self.rustc, self.ipp)
        self.ioc = AppIocHost(ferrite_dir, source_dir, target_dir, self.epics_base, self.app)
        self.fakedev = Fakedev(source_dir, self.ioc, self.ipp)
        self.all = DictComponent({
            "build": TaskList([self.epics_base.install_task, self.app.build_task, self.ioc.install_task]),
            "test": TaskList([self.ipp.test_task, self.app.test_task, self.fakedev.test_task]),
        })

    def components(self) -> Dict[str, Component]:
        return self.__dict__


class CrossComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        ferrite_dir: Path,
        target_dir: Path,
        host: HostComponents,
        platform: Platform,
    ) -> None:
        self.app_gcc = platform.app.gcc
        self.app_rustc = platform.app.rustc
        self.mcu_gcc = platform.mcu.gcc
        self.freertos = platform.mcu.freertos
        self.epics_base = EpicsBaseCross(target_dir, self.app_gcc, host.epics_base)
        self.app = AppReal(source_dir, target_dir, self.app_rustc, host.ipp)
        self.ioc = AppIocCross(ferrite_dir, source_dir, target_dir, self.epics_base, self.app)
        self.mcu = Mcu(ferrite_dir, source_dir, target_dir, self.mcu_gcc, self.freertos, platform.mcu.deployer, host.ipp)

        build_task = TaskList([self.epics_base.install_task, self.ioc.install_task, self.mcu.build_task])
        deploy_task = TaskList([self.epics_base.deploy_task, self.ioc.deploy_task, self.mcu.deploy_and_reboot_task])
        run_task = TaskWrapper(self.ioc.run_task, [deploy_task])
        self.all = DictComponent({"build": build_task, "deploy": deploy_task, "run": run_task})

    def components(self) -> Dict[str, Component]:
        return self.__dict__


@dataclass
class AllComponents(ComponentGroup):

    def __init__(
        self,
        ferrite_dir: Path,
        source_dir: Path,
        target_dir: Path,
    ):
        self.host = HostComponents(
            ferrite_dir,
            source_dir,
            target_dir,
            HostPlatform(target_dir),
        )

        self.device = CrossComponents(
            ferrite_dir,
            source_dir,
            target_dir,
            self.host,
            Imx8mnPlatform(target_dir),
        )

    def components(self) -> Dict[str, Component]:
        return self.__dict__


def make_components(ferrite_dir: Path, base_dir: Path, target_dir: Path) -> ComponentGroup:
    tree = AllComponents(ferrite_dir / "source", base_dir / "source", target_dir)
    tree._update_names()
    return tree
