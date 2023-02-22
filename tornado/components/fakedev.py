from __future__ import annotations

from dataclasses import dataclass

from ferrite.components.base import task, Component, Context

from tornado.components.ioc import AppIocHost
from tornado.info import path as self_path


@dataclass
class Fakedev(Component):
    ioc: AppIocHost

    @task
    def test(self, ctx: Context) -> None:
        self.ioc.epics_base.install(ctx)
        self.ioc.install(ctx)

        from tornado.fakedev import test
        test.run(
            self_path / "source/common",
            ctx.target_path / self.ioc.epics_base.install_dir,
            ctx.target_path / self.ioc.install_dir,
            self.ioc.arch,
            self.ioc.app.log_env(ctx),
        )
