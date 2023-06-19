from __future__ import annotations
from typing import List, Sequence

import shutil
from pathlib import Path

from vortex.utils.path import TargetPath
from vortex.utils.files import substitute
from vortex.tasks.base import task, Context
from vortex.tasks.epics.epics_base import AbstractEpicsBase, EpicsBaseCross, EpicsBaseHost
from vortex.tasks.epics.ioc import AbstractIoc, IocCross, IocHost

from .base import Linkable


class AppIoc(AbstractIoc):
    def __init__(self, epics_base: AbstractEpicsBase, links: Sequence[Linkable], src: Path, dst: TargetPath):
        super().__init__(src, dst, epics_base)
        self.links = links

    @property
    def name(self) -> str:
        return "Tornado"

    def _dep_paths(self, ctx: Context) -> List[Path]:
        return [
            *super()._dep_paths(ctx),
            *[ctx.target_path / l.lib_dir / l.lib_name for l in self.links],
        ]

    def _store_app_lib(self, ctx: Context) -> None:
        lib_dir = ctx.target_path / self.install_dir / "lib" / self.arch
        lib_dir.mkdir(parents=True, exist_ok=True)
        for link in self.links:
            shutil.copy2(
                ctx.target_path / link.lib_dir / link.lib_name,
                lib_dir / link.lib_name,
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
        for link in self.links:
            link.build(ctx)
        try:
            super().build(ctx)
        finally:
            # Copy App shared lib to the IOC even if IOC wasn't built.
            self._store_app_lib(ctx)


class AppIocHost(AppIoc, IocHost):
    def __init__(self, epics_base: EpicsBaseHost, links: Sequence[Linkable], src: Path, dst: TargetPath):
        super().__init__(epics_base, links, src, dst)


class AppIocCross(AppIoc, IocCross):
    def __init__(self, epics_base: EpicsBaseCross, links: Sequence[Linkable], src: Path, dst: TargetPath):
        super().__init__(epics_base, links, src, dst)
