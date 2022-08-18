from __future__ import annotations
from typing import Callable, Dict, List

from dataclasses import dataclass
from pathlib import Path
from multiprocessing import Process
from time import sleep

from ferrite.components.base import Component, Task, Context
from ferrite.components.rust import CargoBin
from ferrite.ioc.fakedev import test as fakedev_test


class Fakedev(Component):

    @dataclass
    class TestTask(Task):
        owner: Fakedev

        def run(self, ctx: Context) -> None:
            self.owner.test()

        def dependencies(self) -> List[Task]:
            return [self.owner.app.build_task]

    def __init__(
        self,
        app: CargoBin,
    ):
        self.app = app
        self.test_task = self.TestTask(self)

    def test(self) -> None:
        fakedev_test(self.app.bin_dir() / "debug/ferrite-app-example")

    def tasks(self) -> Dict[str, Task]:
        return {"test": self.test_task}
