from __future__ import annotations
import os
import shutil
from utils.files import substitute
from manage.components.base import Component, Task, Context
from manage.components.git import Repo
from manage.components.toolchains import Toolchain
from manage.components.app import App
from manage.paths import BASE_DIR, TARGET_DIR
from .base import EpicsBuildTask, epics_arch, epics_host_arch
from .epics_base import EpicsBase

class IocBuildTask(EpicsBuildTask):
    def __init__(
        self,
        src_dir: str,
        build_dir: str,
        install_dir: str,
        deps: list[Task],
        epics_base_dir: str,
        app_src_dir: str,
        app_build_dir: str,
        app_fakedev: bool,
        toolchain: Toolchain,
    ):
        super().__init__(
            src_dir,
            build_dir,
            install_dir,
            deps=deps,
        )
        self.epics_base_dir = epics_base_dir
        self.app_src_dir = app_src_dir
        self.app_build_dir = app_build_dir
        self.app_fakedev = app_fakedev
        self.toolchain = toolchain

    def _configure(self):
        arch = epics_arch(self.epics_base_dir, self.toolchain and self.toolchain.target)

        substitute([
            ("^\\s*#*(\\s*EPICS_BASE\\s*=).*$", f"\\1 {self.epics_base_dir}"),
        ], os.path.join(self.build_dir, "configure/RELEASE"))

        substitute([
            ("^\\s*#*(\\s*APP_SRC_DIR\\s*=).*$", f"\\1 {self.app_src_dir}"),
            ("^\\s*#*(\\s*APP_BUILD_DIR\\s*=).*$", f"\\1 {self.app_build_dir}"),
        ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

        if self.toolchain:
            substitute([
                ("^\\s*#*(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {arch}"),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))
    
        substitute([
            ("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {arch}"),
            ("^\\s*#*(\\s*APP_FAKEDEV\\s*=).*$", f"\\1 {'1' if self.app_fakedev else ''}"),
        ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

        substitute([
            ("^\\s*#*(\\s*INSTALL_LOCATION\\s*=).*$", f"\\1 {self.install_dir}"),
        ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))
        os.makedirs(self.install_dir, exist_ok=True)

        lib_dir = os.path.join(self.install_dir, "lib", arch)
        lib_name = "libapp{}.so".format("_fakedev" if self.app_fakedev else "")
        os.makedirs(lib_dir, exist_ok=True)
        shutil.copy2(
            os.path.join(self.app_build_dir, lib_name),
            os.path.join(lib_dir, lib_name),
        )

    def _install(self):
        shutil.copytree(
            os.path.join(self.build_dir, "iocBoot"),
            os.path.join(self.install_dir, "iocBoot"),
            dirs_exist_ok=True,
            ignore=shutil.ignore_patterns("O.*"),
        )

class IocTestFakeDevTask(Task):
    def __init__(self, owner):
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> bool:
        import ioc.tests.fakedev as fakedev
        epics_base_dir = self.owner.epics_base.paths["host_install"]
        fakedev.run_test(
            self.owner.epics_base.paths["host_install"],
            self.owner.paths["host_install"],
            os.path.join(BASE_DIR, "common"),
            epics_host_arch(self.owner.epics_base.paths["host_build"]),
        )
        return True

    def dependencies(self) -> list[Task]:
        return [self.owner.host_build_task]

class Ioc(Component):
    def __init__(
        self,
        name: str,
        path: str,
        epics_base: EpicsBase,
        app: App,
        cross_toolchain: Toolchain,
    ):
        super().__init__()

        self.name = name
        self.src_path = path
        self.epics_base = epics_base
        self.app = app
        self.cross_toolchain = cross_toolchain

        self.names = {
            "host_build":    f"{self.name}_host_build",
            "cross_build":   f"{self.name}_cross_build",
            "host_install":    f"{self.name}_host_install",
            "cross_install":   f"{self.name}_cross_install",
        }
        self.paths = {k: os.path.join(TARGET_DIR, v) for k, v in self.names.items()}

        self.host_build_task = IocBuildTask(
            self.src_path,
            self.paths["host_build"],
            self.paths["host_install"],
            [self.epics_base.host_build_task, self.app.build_fakedev_task],
            self.epics_base.paths["host_build"],
            self.app.src_dir,
            self.app.host_build_dir,
            True,
            None,
        )
        self.cross_build_task = IocBuildTask(
            self.src_path,
            self.paths["cross_build"],
            self.paths["cross_install"],
            [self.epics_base.cross_build_task, self.app.build_main_cross_task],
            self.epics_base.paths["cross_build"],
            self.app.src_dir,
            self.app.cross_build_dir,
            False,
            self.cross_toolchain,
        )
        self.test_fakedev_task = IocTestFakeDevTask(self)

    def tasks(self) -> dict[str, Task]:
        return {
            "build_host": self.host_build_task,
            "build_cross": self.cross_build_task,
            "test_fakedev": self.test_fakedev_task,
        }

class AppIoc(Ioc):
    def __init__(
        self,
        epics_base: EpicsBase,
        app: App,
        cross_toolchain: Toolchain,
    ):
        super().__init__(
            "ioc",
            os.path.join(BASE_DIR, "ioc"),
            epics_base,
            app,
            cross_toolchain,
        )
