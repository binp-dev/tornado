from __future__ import annotations
from typing import List, TypeVar

import shutil
from pathlib import Path

from ferrite.utils.path import TargetPath
from ferrite.utils.files import substitute
from ferrite.components.base import Context, Task
from ferrite.components.compiler import Gcc
from ferrite.components.app import AppBase
from ferrite.components.epics.epics_base import AbstractEpicsBase
from ferrite.components.epics.ioc import AbstractIoc, AbstractBuildTask, B as B


class AppIoc(AbstractIoc[B]):

    def __init__(
        self,
        ioc_dirs: List[Path],
        target_dir: TargetPath,
        epics_base: B,
        app: AppBase,
    ):
        super().__init__(
            ioc_dirs,
            target_dir,
            epics_base,
        )
        self.app = app


O = TypeVar("O", bound=AppIoc[AbstractEpicsBase[Gcc]], covariant=True)


class AppBuildTask(AbstractBuildTask[O]):

    def __init__(
        self,
        owner: O,
        app_lib_name: str = "libapp.so",
    ):
        super().__init__(owner)
        self.app_lib_name = app_lib_name

    @property
    def app_lib_path(self) -> TargetPath:
        return self.owner.app.bin_dir / self.app_lib_name

    def _dep_paths(self, ctx: Context) -> List[Path]:
        return [
            *super()._dep_paths(ctx),
            ctx.target_path / self.app_lib_path,
        ]

    def _store_app_lib(self, ctx: Context) -> None:
        lib_dir = ctx.target_path / self.owner.install_dir / "lib" / self.owner.arch
        lib_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(
            ctx.target_path / self.app_lib_path,
            lib_dir / self.app_lib_name,
        )

    def _configure(self, ctx: Context) -> None:
        super()._configure(ctx)

        substitute(
            [("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {self.owner.arch}")],
            ctx.target_path / self.owner.build_dir / "configure/CONFIG_SITE.local",
        )

        self._store_app_lib(ctx)

    def run(self, ctx: Context) -> None:
        super().run(ctx)

        # Copy App shared lib to the IOC even if IOC wasn't built.
        self._store_app_lib(ctx)

    def dependencies(self) -> List[Task]:
        return [
            *super().dependencies(),
            self.owner.app.build_task,
        ]
