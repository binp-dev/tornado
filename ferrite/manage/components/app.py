from __future__ import annotations
import os
from utils.run import run
from manage.paths import BASE_DIR, TARGET_DIR
from manage.components.base import Component, Task, TaskWrapper
from manage.components.cmake import Cmake
from manage.components.epics.epics_base import EpicsBase
from manage.components.toolchains import Toolchain

class AppBuildUnittestTask(Task):
    def __init__(self, cmake: Cmake):
        super().__init__()
        self.cmake = cmake

    def run(self, ctx: Context) -> bool:
        self.cmake.configure()
        return self.cmake.build("app_unittest")

class AppRunUnittestTask(Task):
    def __init__(self, cmake: Cmake, build_task: Task):
        super().__init__()
        self.cmake = cmake
        self.build_task = build_task

    def run(self, ctx: Context) -> bool:
        run(["./app_unittest"], cwd=self.cmake.build_dir)
        return True

    def dependencies(self) -> list[Task]:
        return [self.build_task]

class AppBuildWithEpicsTask(Task):
    def __init__(
        self,
        cmake: Cmake,
        cmake_target: str,
        epics_base: EpicsBase,
        toolchain: Toolchain,
        deps: list[Task] = [],
    ):
        super().__init__()
        self.cmake = cmake
        self.cmake_target = cmake_target
        self.epics_base = epics_base
        self.toolchain = toolchain
        self.deps = deps

    def run(self, ctx: Context) -> bool:
        if self.toolchain is None:
            self.cmake.configure({
                "EPICS_BASE": self.epics_base.paths["host_install"],
            })
        else:
            assert self.toolchain == self.epics_base.cross_toolchain
            self.cmake.configure({
                "TOOLCHAIN_DIR": self.toolchain.path,
                "TARGET_TRIPLE": self.toolchain.target,
                "CMAKE_TOOLCHAIN_FILE": os.path.join(self.cmake.src_dir, "armgcc.cmake"),
                "EPICS_BASE": self.epics_base.paths["cross_install"],
            })
        return self.cmake.build(self.cmake_target)

    def dependencies(self) -> list[Task]:
        deps = list(self.deps)
        if self.toolchain is None:
            deps.append(self.epics_base.host_build_task)
        else:
            deps.append(self.epics_base.cross_build_task)
        return deps

class App(Component):
    def __init__(
        self,
        epics_base: EpicsBase,
        cross_toolchain: Toolchain = None,
    ):
        super().__init__()

        self.src_dir = os.path.join(BASE_DIR, "app")
        self.host_build_dir = os.path.join(TARGET_DIR, "app_host")
        self.cross_build_dir = os.path.join(TARGET_DIR, "app_cross")
        self.epics_base = epics_base
        self.cross_toolchain = cross_toolchain

        self.host_cmake = Cmake(self.src_dir, self.host_build_dir)
        self.cross_cmake = Cmake(self.src_dir, self.cross_build_dir)

        self.build_unittest_task = AppBuildUnittestTask(self.host_cmake)
        self.run_unittest_task = AppRunUnittestTask(self.host_cmake, self.build_unittest_task)
        self.build_fakedev_task = AppBuildWithEpicsTask(
            self.host_cmake,
            "app_fakedev",
            self.epics_base,
            None,
        )
        self.build_main_host_task = AppBuildWithEpicsTask(
            self.host_cmake,
            "app",
            self.epics_base,
            None,
        )
        self.build_main_cross_task = AppBuildWithEpicsTask(
            self.cross_cmake,
            "app",
            self.epics_base,
            self.cross_toolchain,
        )

    def tasks(self) -> dict[str, Task]:
        return {
            "build_unittest": self.build_unittest_task,
            "run_unittest": self.run_unittest_task,
            "build_fakedev": self.build_fakedev_task,
            "build_main_host": self.build_main_host_task,
            "build_main_cross": self.build_main_cross_task,
        }
