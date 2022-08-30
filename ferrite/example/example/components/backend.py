from __future__ import annotations
from typing import Dict, List

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Component, Task, Context

from example.components.ioc import AppIocHost
from example.components.protocol import Protocol
import example.backend as backend


class Backend(Component):

    @dataclass(eq=False)
    class BaseTask(Task):
        owner: Backend

        def dependencies(self) -> List[Task]:
            return [
                self.owner.app_ioc.build_task,
                self.owner.proto.generate_task,
            ]

    class TestTask(BaseTask):

        def run(self, ctx: Context) -> None:
            self.owner.test()

        def dependencies(self) -> List[Task]:
            return super().dependencies()

    def __init__(self, app_ioc: AppIocHost, proto: Protocol):
        self.app_ioc = app_ioc
        self.proto = proto
        self.test_task = self.TestTask(self)

    def test(self) -> None:
        backend.test(
            self.app_ioc.epics_base.install_path,
            self.app_ioc.install_path,
            self.app_ioc.arch,
        )

    def tasks(self) -> Dict[str, Task]:
        return {
            "test": self.test_task,
        }
