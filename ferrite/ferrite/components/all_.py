from __future__ import annotations
from typing import Dict

from dataclasses import dataclass

from ferrite.components.base import Component, Task, TaskWrapper
from ferrite.components.epics.epics_base import EpicsBaseCross, EpicsBaseHost
from ferrite.components.codegen import Codegen
from ferrite.components.app import App, AppTest
from ferrite.components.ipp import Ipp
from ferrite.components.epics.ioc import AppIoc
from ferrite.components.mcu import Mcu
from ferrite.remote.tasks import RebootTask


@dataclass
class AllHost(Component):

    epics_base: EpicsBaseHost
    codegen: Codegen
    ipp: Ipp
    app_test: AppTest
    app: App
    ioc: AppIoc

    def __post_init__(self) -> None:
        self.build_task = TaskWrapper(
            deps=[
                self.epics_base.build_task,
                self.codegen.build_task,
                self.ipp.build_task,
                self.app_test.build_task,
                self.app.build_task,
                self.ioc.build_task,
            ],
        )
        self.test_task = TaskWrapper(
            deps=[
                self.codegen.test_task,
                self.ipp.test_task,
                self.app_test.test_task,
                self.ioc.test_fakedev_task,
            ],
        )

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
            "test": self.test_task,
        }


@dataclass
class AllCross(Component):

    epics_base: EpicsBaseCross
    app: App
    ioc: AppIoc
    mcu: Mcu

    def __post_init__(self) -> None:
        self.build_task = TaskWrapper(
            deps=[
                self.mcu.build_task,
                self.epics_base.build_task,
                self.ioc.build_task,
            ],
        )
        self.deploy_task = TaskWrapper(
            deps=[
                self.mcu.deploy_task,
                self.epics_base.deploy_task,
                self.ioc.deploy_task,
            ],
        )
        self.deploy_and_reboot_task = TaskWrapper(
            RebootTask(),
            deps=[self.deploy_task],
        )
        self.run_task = TaskWrapper(
            self.ioc.run_task,
            deps=[self.deploy_and_reboot_task],
        )

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
            "deploy": self.deploy_task,
            "deploy_and_reboot": self.deploy_and_reboot_task,
            "run": self.run_task,
        }
