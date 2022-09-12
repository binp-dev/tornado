from __future__ import annotations
from typing import Dict, List

from ferrite.components.base import Component, Task, OwnedTask, Context

from example.components.frontend import FrontendHost
from example.components.protocol import Protocol
import example.backend as backend


class TestBackend(Component):

    def __init__(self, frontend: FrontendHost, proto: Protocol):
        self.frontend = frontend
        self.proto = proto
        self.test_task = _TestTask(self)

    def test(self) -> None:
        backend.test(
            self.frontend.epics_base.install_path,
            self.frontend.install_path,
            self.frontend.arch,
        )


class _TestTask(OwnedTask[TestBackend]):

    def run(self, ctx: Context) -> None:
        self.owner.test()

    def dependencies(self) -> List[Task]:
        return [
            self.owner.frontend.epics_base.build_task,
            self.owner.frontend.build_task,
            self.owner.proto.generate_task,
        ]
