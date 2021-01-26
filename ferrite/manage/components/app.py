from __future__ import annotations
import os
from manage.paths import BASE_DIR, TARGET_DIR
from manage.components.base import Component, Task
from manage.components.cmake import Cmake
from manage.utils.run import run

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
    def __init__(self, host_cmake: Cmake):
        super().__init__()
        self.host_cmake = host_cmake

    def run(self, ctx: Context) -> bool:
        self.host_cmake.configure()
        return self.host_cmake.build("app_fakedev")

class App(Component):
    def __init__(self, cross_toolchain=None):
        super().__init__()

        self.src_dir = os.path.join(BASE_DIR, "app")
        self.host_build_dir = os.path.join(TARGET_DIR, "app_host")
        self.cross_toolchain = cross_toolchain

        self.host_cmake = Cmake(self.src_dir, self.host_build_dir)

        self.build_unittest_task = AppBuildUnittestTask(self.host_cmake)
        self.run_unittest_task = AppRunUnittestTask(self.host_cmake, self.build_unittest_task)
        self.build_fakedev_task = AppBuildFakedevTask(self.host_cmake)

    def tasks(self) -> dict[str, Task]:
        return {
            "build_unittest": self.build_unittest_task,
            "run_unittest": self.run_unittest_task,
            "build_fakedev": self.build_fakedev_task,
        }
