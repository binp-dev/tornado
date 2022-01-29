from __future__ import annotations
from typing import Callable, Dict, List

import shutil
from pathlib import Path

from ferrite.utils.files import substitute
from ferrite.components.base import Task, Context
from ferrite.components.toolchain import Toolchain, HostToolchain
from ferrite.components.app import AppBase
from ferrite.components.epics.base import epics_arch, epics_host_arch
from ferrite.components.epics.epics_base import EpicsBase, EpicsBaseHost
from ferrite.components.epics.ioc import Ioc, IocCross, IocHost


class AppIoc(Ioc):

    class BuildTask(Ioc.BuildTask):

        APP_LIB_NAME: str = "libapp.so"

        def __init__(
            self,
            owner: AppIoc,
            deps: List[Task],
            app_lib_name: str = APP_LIB_NAME,
        ):
            self._app_owner = owner
            super().__init__(owner, deps=deps)

            self.app_src_dir = self.owner.app.src_dir
            self.app_build_dir = self.owner.app.build_dir
            self.app_lib_name = app_lib_name

        @property
        def owner(self) -> AppIoc:
            return self._app_owner

        def _configure(self) -> None:
            super()._configure()

            arch = epics_arch(self.epics_base_dir, self.owner.toolchain)
            substitute(
                [
                    ("^\\s*#*(\\s*APP_SRC_DIR\\s*=).*$", f"\\1 {self.app_src_dir}"),
                    ("^\\s*#*(\\s*APP_BUILD_DIR\\s*=).*$", f"\\1 {self.app_build_dir}"),
                    ("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {arch}"),
                ],
                self.build_dir / "configure/CONFIG_SITE.local",
            )

            lib_dir = self.install_dir / "lib" / arch
            lib_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(
                self.app_build_dir / self.app_lib_name,
                lib_dir / self.app_lib_name, # self.APP_LIB_NAME
            )

    def _build_deps(self) -> List[Task]:
        deps = super()._build_deps()
        deps.append(self.app.build_task)
        return deps

    def _make_build_task(self) -> AppIoc.BuildTask:
        return self.BuildTask(self, deps=self._build_deps())

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        epics_base: EpicsBase,
        app: AppBase,
    ):
        self.app = app
        super().__init__(
            "ioc",
            source_dir / "ioc",
            target_dir,
            epics_base,
        )


class AppIocHost(AppIoc, IocHost):

    class RunTask(Task):

        def __init__(self, owner: AppIocHost, run_fn: Callable[[Path, Path, str], None]) -> None:
            super().__init__()
            self.owner = owner
            self.run_fn = run_fn

        def run(self, ctx: Context) -> None:
            self.run_fn(
                self.owner.epics_base.install_path,
                self.owner.install_path,
                epics_host_arch(self.owner.epics_base.build_path),
            )

        def dependencies(self) -> List[Task]:
            return [
                self.owner.epics_base.build_task,
                self.owner.build_task,
            ]

    def _make_build_task(self) -> AppIoc.BuildTask:
        return self.BuildTask(
            self,
            deps=self._build_deps(),
            app_lib_name="libapp_fakedev.so",
        )

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        epics_base: EpicsBase,
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


class AppIocCross(AppIoc, IocCross):
    pass
