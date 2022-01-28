from __future__ import annotations
from typing import Dict

from dataclasses import dataclass

from ferrite.components.base import Component, Task, TaskWrapper
from ferrite.components.codegen import Codegen
from ferrite.components.app import AppTest


@dataclass
class All(Component):

    codegen: Codegen
    app_test: AppTest

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

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
            "test": self.test_task,
        }
