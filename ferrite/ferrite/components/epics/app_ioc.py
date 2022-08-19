from __future__ import annotations
from typing import List

import shutil
from pathlib import Path

from ferrite.utils.files import substitute
from ferrite.components.base import Context, Task
from ferrite.components.app import AppBase
from ferrite.components.epics.epics_base import AbstractEpicsBase
from ferrite.components.epics.ioc import AbstractIoc, IocHost


class AppIoc(AbstractIoc):

    class BuildTask(AbstractIoc.BuildTask):

        def __init__(
            self,
            owner: AppIoc,
            deps: List[Task],
            app_lib_name: str = "libapp.so",
        ):
            self._app_owner = owner
            super().__init__(owner, deps=deps)

            self.app_build_dir = self.owner.app.build_dir
            self.app_lib_name = app_lib_name
            self.app_lib_path = self.owner.app.bin_dir / self.app_lib_name

        @property
        def owner(self) -> AppIoc:
            return self._app_owner

        def _dep_paths(self) -> List[Path]:
            return [
                *super()._dep_paths(),
                self.app_lib_path,
            ]

        def _store_app_lib(self) -> None:
            lib_dir = self.install_dir / "lib" / self.owner.arch
            lib_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(
                self.app_lib_path,
                lib_dir / self.app_lib_name,
            )

        def _configure(self) -> None:
            super()._configure()

            substitute(
                [("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {self.owner.arch}")],
                self.build_dir / "configure/CONFIG_SITE.local",
            )

            self._store_app_lib()

        def run(self, ctx: Context) -> None:
            super().run(ctx)

            # Copy App shared lib to the IOC even if IOC wasn't built.
            self._store_app_lib()

    def _build_deps(self) -> List[Task]:
        deps = super()._build_deps()
        deps.append(self.app.build_task)
        return deps

    def _make_build_task(self) -> AppIoc.BuildTask:
        return self.BuildTask(self, deps=self._build_deps())

    def __init__(
        self,
        ioc_dirs: List[Path],
        target_dir: Path,
        epics_base: AbstractEpicsBase,
        app: AppBase,
    ):
        self.app = app

        super().__init__(
            "ioc",
            ioc_dirs,
            target_dir,
            epics_base,
        )


class AppIocExample(AppIoc, IocHost):

    class BuildTask(AppIoc.BuildTask, IocHost.BuildTask):
        pass
