from __future__ import annotations
import os
from utils.run import run
from manage.paths import BASE_DIR, TARGET_DIR
from manage.components.base import Component, Task
from manage.components.cmake import Cmake
from manage.components.epics.epics_base import EpicsBase
from manage.components.toolchains import Toolchain

class AppBuildUnittestTask(Task):
    def __init__(self, host_cmake: Cmake):
        super().__init__()
        self.host_cmake = host_cmake

    def run(self, ctx: Context) -> bool:
        self.host_cmake.configure()
        return self.host_cmake.build("app_unittest")

class AppRunUnittestTask(Task):
    def __init__(self, host_cmake: Cmake, build_task: Task):
        super().__init__()
        self.host_cmake = host_cmake
        self.build_task = build_task

    def run(self, ctx: Context) -> bool:
        run(["./app_unittest"], cwd=self.host_cmake.build_dir)
        return True

    def dependencies(self) -> list[Task]:
        return [self.build_task]

class AppBuildFakedevTask(Task):
    def __init__(self, host_cmake: Cmake, epics_base: EpicsBase):
        super().__init__()
        self.host_cmake = host_cmake
        self.epics_base = epics_base

    def run(self, ctx: Context) -> bool:
        self.host_cmake.configure({
            "EPICS_BASE": self.epics_base.paths["host_build"],
        })
        return self.host_cmake.build("app_fakedev")

    def dependencies(self) -> list[Task]:
        return [self.epics_base.host_build_task]

class App(Component):
    def __init__(
        self,
        epics_base: EpicsBase,
        cross_toolchain: Toolchain = None,
    ):
        super().__init__()

        self.src_dir = os.path.join(BASE_DIR, "app")
        self.host_build_dir = os.path.join(TARGET_DIR, "app_host")
        self.epics_base = epics_base
        self.cross_toolchain = cross_toolchain

        self.host_cmake = Cmake(self.src_dir, self.host_build_dir)

        self.build_unittest_task = AppBuildUnittestTask(self.host_cmake)
        self.run_unittest_task = AppRunUnittestTask(self.host_cmake, self.build_unittest_task)
        self.build_fakedev_task = AppBuildFakedevTask(self.host_cmake, self.epics_base)

    def tasks(self) -> dict[str, Task]:
        return {
            "build_unittest": self.build_unittest_task,
            "run_unittest": self.run_unittest_task,
            "build_fakedev": self.build_fakedev_task,
        }
