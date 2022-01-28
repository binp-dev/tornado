from __future__ import annotations
from typing import Dict, List

import shutil
from pathlib import Path

from ferrite.utils.files import substitute
from ferrite.components.base import Task, Context
from ferrite.components.toolchain import Toolchain, HostToolchain
from ferrite.components.app import AppBase
from ferrite.components.epics.base import epics_arch, epics_host_arch
from ferrite.components.epics.epics_base import EpicsBase, EpicsBaseHost
from ferrite.components.epics.ioc import Ioc, IocBuildTask


class AppIocBuildTask(IocBuildTask):

    def __init__(
        self,
        src_dir: Path,
        build_dir: Path,
        install_dir: Path,
        deps: List[Task],
        epics_base_dir: Path,
        app_src_dir: Path,
        app_build_dir: Path,
        toolchain: Toolchain,
        app_lib_name: str = "libapp.so",
    ):
        super().__init__(
            src_dir,
            build_dir,
            install_dir,
            deps,
            epics_base_dir,
            toolchain,
        )

        self.app_src_dir = app_src_dir
        self.app_build_dir = app_build_dir
        self.app_lib_name = app_lib_name

    def _configure(self) -> None:
        super()._configure()

        arch = epics_arch(self.epics_base_dir, self.toolchain)
        substitute(
            [
                ("^\\s*#*(\\s*APP_SRC_DIR\\s*=).*$", f"\\1 {self.app_src_dir}"),
                ("^\\s*#*(\\s*APP_BUILD_DIR\\s*=).*$", f"\\1 {self.app_build_dir}"),
                ("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {arch}"),
            ],
            self.build_dir / "configure/CONFIG_SITE",
        )

        lib_dir = self.install_dir / "lib" / arch
        lib_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(
            self.app_build_dir / self.app_lib_name,
            lib_dir / self.app_lib_name,
        )


class AppIocFakeDevBuildTask(AppIocBuildTask):

    def __init__(
        self,
        src_dir: Path,
        build_dir: Path,
        install_dir: Path,
        deps: List[Task],
        epics_base_dir: Path,
        app_src_dir: Path,
        app_build_dir: Path,
        toolchain: Toolchain,
        app_fakedev: bool,
    ):
        super().__init__(
            src_dir,
            build_dir,
            install_dir,
            deps,
            epics_base_dir,
            app_src_dir,
            app_build_dir,
            toolchain,
            app_lib_name=["libapp.so", "libapp_fakedev.so"][app_fakedev],
        )

        self.app_src_dir = app_src_dir
        self.app_build_dir = app_build_dir
        self.app_fakedev = app_fakedev

    def _configure(self) -> None:
        super()._configure()

        if self.app_fakedev:
            substitute(
                [("^\\s*#*(\\s*APP_FAKEDEV\\s*=).*$", f"\\1 1")],
                self.build_dir / "configure/CONFIG_SITE.local",
            )


class AppIocFakeDevTestTask(Task):

    def __init__(self, owner: Ioc) -> None:
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> None:
        import ferrite.ioc.fakedev.test as fakedev
        fakedev.run(
            self.owner.epics_base.paths["install"],
            self.owner.paths["install"],
            epics_host_arch(self.owner.epics_base.paths["build"]),
        )

    def dependencies(self) -> List[Task]:
        return [
            self.owner.epics_base.tasks()["build"],
            self.owner.build_task,
        ]


class AppIocFakeDevRunTask(Task):

    def __init__(self, owner: Ioc) -> None:
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> None:
        import ferrite.ioc.fakedev.dummy as fakedev
        fakedev.run(
            self.owner.epics_base.paths["install"],
            self.owner.paths["install"],
            epics_host_arch(self.owner.epics_base.paths["build"]),
        )

    def dependencies(self) -> List[Task]:
        return [
            self.owner.epics_base.tasks()["build"],
            self.owner.build_task,
        ]


class AppIoc(Ioc):

    def _build_deps(self) -> List[Task]:
        deps = super()._build_deps()
        deps.append(self.app.build_task)
        return deps

    def _make_build_task(self) -> AppIocBuildTask:
        return AppIocFakeDevBuildTask(
            self.src_path,
            self.paths["build"],
            self.paths["install"],
            self._build_deps(),
            self.epics_base.paths["build"],
            self.app.src_dir,
            self.app.build_dir,
            self.toolchain,
            isinstance(self.epics_base, EpicsBaseHost),
        )

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        epics_base: EpicsBase,
        app: AppBase,
        toolchain: Toolchain,
    ):
        self.app = app

        super().__init__(
            "ioc",
            source_dir / "ioc",
            target_dir,
            epics_base,
            toolchain,
        )

        if isinstance(self.epics_base, EpicsBaseHost):
            self.run_fakedev_task = AppIocFakeDevRunTask(self)
            self.test_fakedev_task = AppIocFakeDevTestTask(self)

    def tasks(self) -> Dict[str, Task]:
        tasks = super().tasks()

        if isinstance(self.toolchain, HostToolchain):
            tasks.update({
                "run_fakedev": self.run_fakedev_task,
                "test_fakedev": self.test_fakedev_task,
            })

        return tasks
