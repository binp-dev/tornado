from __future__ import annotations
from typing import Dict

from dataclasses import dataclass

from ferrite.components.base import Component, Task, TaskWrapper
from ferrite.components.epics.app_ioc import AppIocExample
from ferrite.components.epics.epics_base import EpicsBaseHost
from ferrite.components.codegen import CodegenWithTest
from ferrite.components.fakedev import Fakedev
from ferrite.components.app import AppBase


@dataclass
class AllHost(Component):

    epics_base: EpicsBaseHost
    codegen: CodegenWithTest
    app: AppBase
    ioc_example: AppIocExample
    fakedev: Fakedev

    def __post_init__(self) -> None:
        self.build_task = TaskWrapper(
            deps=[
                self.epics_base.build_task,
                self.codegen.build_task,
                self.app.build_task,
                self.ioc_example.build_task,
            ],
        )
        self.test_task = TaskWrapper(
            deps=[
                self.codegen.test_task,
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

    app: AppBase

    def __post_init__(self) -> None:
        self.build_task = TaskWrapper(deps=[self.app.build_task],)

    def tasks(self) -> Dict[str, Task]:
        return {"build": self.build_task}
