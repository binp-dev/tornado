from __future__ import annotations
from typing import Dict

from ferrite.utils.path import TargetPath
from ferrite.components.base import task, Context
from ferrite.components.rust import Cargo, RustcHost
from ferrite.components.concurrency import ConcurrentTaskList

from tornado.components.ioc import AppIocHost
from tornado.info import path as self_path


class Fakedev(Cargo):

    def __init__(self, ioc: AppIocHost, rustc: RustcHost) -> None:
        super().__init__(self_path / "source/fakedev", TargetPath("tornado/fakedev"), rustc)
        self.ioc = ioc

    def env(self, ctx: Context) -> Dict[str, str]:
        epics_base = self.ioc.epics_base
        return {
            **super().env(ctx),
            "EPICS_BASE": str(ctx.target_path / epics_base.install_dir),
            "LD_LIBRARY_PATH": str(ctx.target_path / epics_base.install_dir / "lib" / epics_base.arch),
        }

    @task
    def run(self, ctx: Context) -> None:
        self.ioc.install(ctx)
        self.build(ctx)

        @task
        def fake_ioc(ctx: Context) -> None:
            self.ioc.run(ctx)

        @task
        def fake_mcu(ctx: Context) -> None:
            super(Fakedev, self).run(ctx, bin="run")

        ConcurrentTaskList(fake_ioc, fake_mcu)(ctx)

    @task
    def test(self, ctx: Context) -> None:
        self.ioc.install(ctx)
        self.build(ctx)

        @task
        def fake_ioc(ctx: Context) -> None:
            self.ioc.run(ctx)

        @task
        def fake_mcu(ctx: Context) -> None:
            super(Fakedev, self).run(ctx, bin="test")

        ConcurrentTaskList(fake_ioc, fake_mcu)(ctx)
