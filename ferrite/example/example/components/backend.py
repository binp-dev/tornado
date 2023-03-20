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

    def test(self, ctx: Context) -> None:
        backend.test(
            ctx.target_path / self.frontend.epics_base.install_dir,
            ctx.target_path / self.frontend.install_dir,
            self.frontend.arch,
            env={**self.frontend.app.log_env(ctx)},
        )


class _TestTask(OwnedTask[TestBackend]):

    def run(self, ctx: Context) -> None:
        self.owner.test(ctx)

    def dependencies(self) -> List[Task]:
        return [
            self.owner.frontend.epics_base.install_task,
            self.owner.frontend.install_task,
            self.owner.proto.generate_task,
        ]
