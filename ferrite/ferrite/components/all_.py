from __future__ import annotations
from typing import Dict

from dataclasses import dataclass

from ferrite.components.base import Component, Task, TaskWrapper
from ferrite.components.core import CoreTest
from ferrite.components.codegen import CodegenWithTest
from ferrite.components.app import AppBaseTest, AppExample


@dataclass
class AllHost(Component):

    core_test: CoreTest
    codegen: CodegenWithTest
    app_base_test: AppBaseTest

    def __post_init__(self) -> None:
        self.build_task = TaskWrapper(
            deps=[
                self.core_test.build_task,
                self.codegen.build_task,
                self.app_base_test.build_task,
            ],
        )
        self.test_task = TaskWrapper(
            deps=[
                self.core_test.run_task,
                self.codegen.test_task,
                self.app_base_test.run_task,
            ],
        )

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
            "test": self.test_task,
        }


@dataclass
class AllCross(Component):

    app_example: AppExample

    def __post_init__(self) -> None:
        self.build_task = TaskWrapper(
            deps=[
                self.app_example.build_task,
            ],
        )

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
        }
