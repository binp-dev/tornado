from __future__ import annotations
from typing import List, Callable

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Component, Task, OwnedTask, Context

from tornado.components.ioc import AppIocHost
from tornado.components.ipp import Ipp


@dataclass
class Fakedev(Component):
    source_dir: Path
    ioc: AppIocHost
    ipp: Ipp

    def __post_init__(self) -> None:
        from tornado.fakedev import dummy, test
        self.run_task = _RunTask(self, dummy.run)
        self.test_task = _RunTask(self, test.run)


@dataclass
class _RunTask(OwnedTask[Fakedev]):
    run_fn: Callable[[Path, Path, Path, str], None]

    def run(self, ctx: Context) -> None:
        self.run_fn(
            self.owner.source_dir,
            self.owner.ioc.epics_base.install_path,
            self.owner.ioc.install_path,
            self.owner.ioc.arch,
        )

    def dependencies(self) -> List[Task]:
        return [
            self.owner.ioc.epics_base.install_task,
            self.owner.ioc.install_task,
            self.owner.ipp.generate_task,
        ]
