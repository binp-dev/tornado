from __future__ import annotations
from typing import Dict

from dataclasses import dataclass

from ferrite.components.base import Component, Task, TaskWrapper
from ferrite.components.epics.epics_base import EpicsBaseCross, EpicsBaseHost
from ferrite.components.codegen import CodegenExample
from ferrite.components.app import AppBase
from ferrite.components.epics.ioc_example import IocHostExample, IocCrossExample
from ferrite.components.fakedev import Fakedev


@dataclass
class AllHost(Component):

    epics_base: EpicsBaseHost
    codegen: CodegenExample
    app: AppBase
    ioc_example: IocHostExample
    fakedev: Fakedev

    def __post_init__(self) -> None:
        self.build_task = TaskWrapper(
            deps=[
                self.epics_base.build_task,
                self.codegen.generate_task,
                self.app.build_task,
                self.ioc_example.build_task,
            ],
        )
        self.test_task = TaskWrapper(
            deps=[
                self.app.test_task,
                self.fakedev.test_task,
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
    app: AppBase
    ioc: IocCrossExample

    def __post_init__(self) -> None:
        self.build_task = TaskWrapper(
            deps=[
                self.epics_base.build_task,
                self.ioc.build_task,
            ],
        )
        self.deploy_task = TaskWrapper(
            deps=[
                self.epics_base.deploy_task,
                self.ioc.deploy_task,
            ],
        )
        self.run_task = TaskWrapper(
            self.ioc.run_task,
            deps=[self.deploy_task],
        )

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
            "deploy": self.deploy_task,
            "run": self.run_task,
        }
