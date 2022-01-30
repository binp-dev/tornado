from __future__ import annotations
from typing import Callable, Dict, List

import shutil
from pathlib import Path

from ferrite.utils.files import substitute
from ferrite.components.base import Task, Context
from ferrite.components.app import AppBase
from ferrite.components.epics.epics_base import AbstractEpicsBase
from ferrite.components.epics.ioc import AbstractIoc, IocCross, IocHost


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
                    ("^\\s*#*(\\s*APP_SRC_DIR\\s*=).*$", f"\\1 {self.app_src_dir}"),
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
        source_dir: Path,
        target_dir: Path,
        epics_base: AbstractEpicsBase,
        app: AppBase,
    ):
        self.app = app
        super().__init__(
            "ioc",
            source_dir / "ioc",
            target_dir,
            epics_base,
        )


class AppIocHost(AbstractAppIoc, IocHost):

    class BuildTask(AbstractAppIoc.BuildTask, IocHost.BuildTask):
        pass

    class RunTask(Task):

        def __init__(self, owner: AppIocHost, run_fn: Callable[[Path, Path, str], None]) -> None:
            super().__init__()
            self.owner = owner
            self.run_fn = run_fn

        def run(self, ctx: Context) -> None:
            self.run_fn(
                self.owner.epics_base.install_path,
                self.owner.install_path,
                self.owner.arch,
            )

        def dependencies(self) -> List[Task]:
            return [
                self.owner.epics_base.build_task,
                self.owner.build_task,
            ]

    def _make_build_task(self) -> AbstractAppIoc.BuildTask:
        return self.BuildTask(
            self,
            deps=self._build_deps(),
            app_lib_name="libapp_fakedev.so",
        )

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        epics_base: AbstractEpicsBase,
        app: AppBase,
    ):
        self.app = app

        super().__init__(
            source_dir,
            target_dir,
            epics_base,
            app,
        )

        from ferrite.ioc.fakedev import dummy, test
        self.run_fakedev_task = self.RunTask(self, dummy.run)
        self.test_fakedev_task = self.RunTask(self, test.run)

    def tasks(self) -> Dict[str, Task]:
        tasks = super().tasks()
        tasks.update({
            "run_fakedev": self.run_fakedev_task,
            "test_fakedev": self.test_fakedev_task,
        })
        return tasks


class AppIocCross(AbstractAppIoc, IocCross):

    class BuildTask(AbstractAppIoc.BuildTask, IocCross.BuildTask):
        pass
