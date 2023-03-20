from __future__ import annotations
from typing import List

import shutil
from pathlib import Path

from ferrite.utils.files import substitute
from ferrite.components.base import Task
from ferrite.components.app import AppBase
from ferrite.components.epics.epics_base import AbstractEpicsBase
from ferrite.components.epics.ioc import AbstractIoc


class AbstractAppIoc(AbstractIoc):

    class BuildTask(AbstractIoc.BuildTask):

        def __init__(
            self,
            owner: AbstractAppIoc,
            deps: List[Task],
            app_lib_name: str = "libapp.so",
        ):
            self._app_owner = owner
            super().__init__(owner, deps=deps)

            self.app_lib_src_dir = self.owner.app.lib_src_dir
            self.app_src_dir = self.owner.app.src_dir
            self.app_build_dir = self.owner.app.build_dir
            self.app_lib_name = app_lib_name

        @property
        def owner(self) -> AbstractAppIoc:
            return self._app_owner

        def _configure(self) -> None:
            super()._configure()

            substitute(
                [
                    ("^\\s*#*(\\s*APP_LIB_SRC\\s*=).*$", f"\\1 {self.app_lib_src_dir}"),
                    ("^\\s*#*(\\s*APP_BUILD_DIR\\s*=).*$", f"\\1 {self.app_build_dir}"),
                    ("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {self.owner.arch}"),
                ],
                self.build_dir / "configure/CONFIG_SITE.local",
            )

            lib_dir = self.install_dir / "lib" / self.owner.arch
            lib_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(
                self.app_build_dir / self.app_lib_name,
                lib_dir / self.app_lib_name,
            )

    def _build_deps(self) -> List[Task]:
        deps = super()._build_deps()
        deps.append(self.app.build_task)
        return deps

    def _make_build_task(self) -> AbstractAppIoc.BuildTask:
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
