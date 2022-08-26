from __future__ import annotations
from typing import Dict, List

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Component, Task, Context

from ferrite.ioc.fakedev.protocol import Imsg, Omsg
import ferrite.ioc.fakedev.test as test

from example.components.ioc import AppIocHost
from example.components.protocol import Protocol


class Fakedev(Component):

    @dataclass
    class BaseTask(Task):
        owner: Fakedev

        def dependencies(self) -> List[Task]:
            return [self.owner.app_ioc.build_task]

    class TestTask(BaseTask):

        def run(self, ctx: Context) -> None:
            self.owner.test()

        def dependencies(self) -> List[Task]:
            return super().dependencies()

    class RunTask(BaseTask):

        def run(self, ctx: Context) -> None:
            self.owner.run()

    def __init__(self, app_ioc: AppIocHost, protocol: Protocol):
        self.app_ioc = app_ioc
        self.protocol = protocol
        self.test_task = self.TestTask(self)
        self.run_task = self.RunTask(self)

    def test(self) -> None:
        test.test(self.app_ioc.app.bin_dir / "app")

    def run(self) -> None:
        test.run(self.app_ioc.install_path, self.app_ioc.arch)

    def tasks(self) -> Dict[str, Task]:
        return {
            "test": self.test_task,
            "run": self.run_task,
        }
