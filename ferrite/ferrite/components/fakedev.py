from __future__ import annotations
from typing import Dict, List

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Component, Task, Context
from ferrite.components.epics.ioc_example import IocHostExample
from ferrite.components.codegen import CodegenTest
from ferrite.components.rust import RustcHost

from ferrite.codegen.generator import Generator
from ferrite.ioc.fakedev.protocol import Imsg, Omsg
import ferrite.ioc.fakedev.test as test


class Fakedev(Component):

    @dataclass
    class BaseTask(Task):
        owner: Fakedev

        def dependencies(self) -> List[Task]:
            return [
                self.owner.app_ioc.build_task,
                self.owner.protocol.generate_task,
            ]

    class TestTask(BaseTask):

        def run(self, ctx: Context) -> None:
            self.owner.test()

        def dependencies(self) -> List[Task]:
            return [
                *super().dependencies(),
                self.owner.protocol.test_task,
            ]

    class RunTask(BaseTask):

        def run(self, ctx: Context) -> None:
            self.owner.run()

    def __init__(self, app_ioc: IocHostExample, protocol: Protocol):
        self.app_ioc = app_ioc
        self.protocol = protocol
        self.test_task = self.TestTask(self)
        self.run_task = self.RunTask(self)

    def test(self) -> None:
        test.test(self.app_ioc.app.bin_dir / "app-example")

    def run(self) -> None:
        test.run(self.app_ioc.install_path, self.app_ioc.arch)

    def tasks(self) -> Dict[str, Task]:
        return {
            "test": self.test_task,
            "run": self.run_task,
            **{f"protocol.{k}": v for k, v in self.protocol.tasks().items()},
        }


class Protocol(CodegenTest):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        rustc: RustcHost,
    ):
        super().__init__(
            "fakeproto",
            source_dir,
            target_dir / "fakeproto",
            Generator([Imsg, Omsg]),
            rustc,
        )
