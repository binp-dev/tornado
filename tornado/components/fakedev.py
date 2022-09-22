from __future__ import annotations
from typing import List, Callable

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Component, Task, OwnedTask, Context

from tornado.components.ioc import AppIocHost
from tornado.components.ioc import AppIocHost
from tornado.components.ipp import Ipp
from tornado.info import path as self_path


@dataclass
class Fakedev(Component):
    ioc: AppIocHost
    ipp: Ipp

    def __post_init__(self) -> None:
        from tornado.fakedev import dummy, test
        self.run_task = _RunTask(self, dummy.run)
        self.test_task = _RunTask(self, test.run)


@dataclass(eq=False)
class _RunTask(OwnedTask[Fakedev]):
    run_fn: Callable[[Path, Path, Path, str], None]

    def run(self, ctx: Context) -> None:
        self.run_fn(
            self_path / "source/common",
            ctx.target_path / self.owner.ioc.epics_base.install_dir,
            ctx.target_path / self.owner.ioc.install_dir,
            self.owner.ioc.arch,
        )

    def dependencies(self) -> List[Task]:
        return [
            self.owner.ioc.epics_base.install_task,
            self.owner.ioc.install_task,
            self.owner.ipp.generate_task,
        ]
