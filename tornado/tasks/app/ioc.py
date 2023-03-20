from __future__ import annotations
from typing import List

import shutil
from pathlib import Path

from vortex.utils.path import TargetPath
from vortex.utils.files import substitute
from vortex.tasks.base import task, Context
from vortex.tasks.epics.epics_base import AbstractEpicsBase, EpicsBaseCross, EpicsBaseHost
from vortex.tasks.epics.ioc import AbstractIoc, IocCross, IocHost

from tornado.tasks.app.user import AbstractApp, AppReal, AppFake


class AppIoc(AbstractIoc):
    def __init__(self, epics_base: AbstractEpicsBase, app: AbstractApp, src: Path, dst: TargetPath):
        super().__init__(src, dst, epics_base)
        self.app = app

    @property
    def name(self) -> str:
        return "Tornado"

    def _dep_paths(self, ctx: Context) -> List[Path]:
        return [
            *super()._dep_paths(ctx),
            ctx.target_path / self.app.lib_path,
        ]

    def _store_app_lib(self, ctx: Context) -> None:
        lib_dir = ctx.target_path / self.install_dir / "lib" / self.arch
        lib_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(
            ctx.target_path / self.app.lib_path,
            lib_dir / self.app.lib_name,
        )

    def _configure(self, ctx: Context) -> None:
        super()._configure(ctx)

        substitute(
            [("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {self.arch}")],
            ctx.target_path / self.build_dir / "configure/CONFIG_SITE.local",
        )

        self._store_app_lib(ctx)

    @task
    def build(self, ctx: Context) -> None:
        self.app.build(ctx)
        try:
            super().build(ctx)
        finally:
            # Copy App shared lib to the IOC even if IOC wasn't built.
            self._store_app_lib(ctx)


class AppIocHost(AppIoc, IocHost):
    def __init__(self, epics_base: EpicsBaseHost, app: AppFake, src: Path, dst: TargetPath):
        super().__init__(epics_base, app, src, dst)


class AppIocCross(AppIoc, IocCross):
    def __init__(self, epics_base: EpicsBaseCross, app: AppReal, src: Path, dst: TargetPath):
        super().__init__(epics_base, app, src, dst)