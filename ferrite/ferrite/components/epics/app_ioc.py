from __future__ import annotations
from typing import List

import shutil
from pathlib import Path

from ferrite.utils.files import substitute
from ferrite.components.base import Context, Task
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

            self.core_src_dir = self.owner.ferrite_source_dir / "core"
            self.app_base_src_dir = self.owner.ferrite_source_dir / "app" / "base"
            self.app_src_dir = self.owner.app.src_dir
            self.app_build_dir = self.owner.app.build_dir
            self.app_lib_name = app_lib_name

        @property
        def owner(self) -> AbstractAppIoc:
            return self._app_owner

        def _dep_paths(self) -> List[Path]:
            return [
                *super()._dep_paths(),
                self.core_src_dir,
                self.app_base_src_dir,
            ]

        def _store_app_lib(self) -> None:
            lib_dir = self.install_dir / "lib" / self.owner.arch
            lib_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(
                self.app_build_dir / self.app_lib_name,
                lib_dir / self.app_lib_name,
            )

        def _configure(self) -> None:
            super()._configure()

            substitute(
                [
                    ("^\\s*#*(\\s*CORE_SRC\\s*=).*$", f"\\1 {self.core_src_dir}"),
                    ("^\\s*#*(\\s*APP_BASE_SRC\\s*=).*$", f"\\1 {self.app_base_src_dir}"),
                    ("^\\s*#*(\\s*APP_BUILD_DIR\\s*=).*$", f"\\1 {self.app_build_dir}"),
                    ("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {self.owner.arch}"),
                ],
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

    def _make_build_task(self) -> AbstractAppIoc.BuildTask:
        return self.BuildTask(self, deps=self._build_deps())

    def __init__(
        self,
        ioc_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        epics_base: AbstractEpicsBase,
        app: AppBase,
    ):
        self.app = app
        self.ferrite_source_dir = ferrite_source_dir

        super().__init__(
            "ioc",
            [ferrite_source_dir / "ioc", ioc_dir],
            target_dir,
            epics_base,
        )
