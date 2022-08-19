from __future__ import annotations
from typing import Dict, List

from dataclasses import dataclass

from ferrite.components.base import Component, Task, Context
from ferrite.components.epics.ioc_example import IocHostExample
import ferrite.ioc.fakedev as fakedev


class Fakedev(Component):

    @dataclass
    class BaseTask(Task):
        owner: Fakedev

        def dependencies(self) -> List[Task]:
            return [self.owner.app_ioc.build_task]

    class TestTask(BaseTask):

        def run(self, ctx: Context) -> None:
            self.owner.test()

    class RunTask(BaseTask):

        def run(self, ctx: Context) -> None:
            self.owner.run()

    def __init__(self, app_ioc: IocHostExample):
        self.app_ioc = app_ioc
        self.test_task = self.TestTask(self)
        self.run_task = self.RunTask(self)

    def test(self) -> None:
        fakedev.test(self.app_ioc.app.bin_dir / "app-example")

    def run(self) -> None:
        fakedev.run(self.app_ioc.install_path, self.app_ioc.arch)

    def tasks(self) -> Dict[str, Task]:
        return {"test": self.test_task, "run": self.run_task}
