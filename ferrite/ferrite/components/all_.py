from __future__ import annotations
from typing import Dict

from dataclasses import dataclass

from ferrite.components.base import Component, Task, TaskWrapper
from ferrite.components.codegen import CodegenWithTest
from ferrite.components.rust import CargoBin, CargoWithTest


@dataclass
class AllHost(Component):

    codegen: CodegenWithTest
    app_test: CargoWithTest
    app_example: CargoBin

    def __post_init__(self) -> None:
        self.build_task = TaskWrapper(
            deps=[
                self.codegen.build_task,
                self.app_test.build_task,
            ],
        )
        self.test_task = TaskWrapper(
            deps=[
                self.codegen.test_task,
                self.app_test.test_task,
            ],
        )
        self.run_task = TaskWrapper(
            deps=[
                self.codegen.test_task,
                self.app_example.run_task,
            ],
        )

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
            "test": self.test_task,
            "run": self.run_task,
        }


@dataclass
class AllCross(Component):

    app_example: CargoBin

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
