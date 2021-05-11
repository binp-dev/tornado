from __future__ import annotations
import os
from utils.run import run
from manage.paths import BASE_DIR, TARGET_DIR
from manage.components.base import Component, Task, TaskWrapper
from manage.components.cmake import Cmake
from manage.components.epics.epics_base import EpicsBase
from manage.components.toolchains import Toolchain, HostToolchain, RemoteToolchain

class AppBuildUnittestTask(Task):
    def __init__(self, cmake: Cmake):
        super().__init__()
        self.cmake = cmake

    def run(self, ctx: Context) -> bool:
        self.cmake.configure(ctx)
        return self.cmake.build(ctx, "app_unittest")

    def artifacts(self) -> str[list]:
        return [self.cmake.build_dir]

class AppRunUnittestTask(Task):
    def __init__(self, cmake: Cmake, build_task: Task):
        super().__init__()
        self.cmake = cmake
        self.build_task = build_task

    def run(self, ctx: Context) -> bool:
        run(["./app_unittest"], cwd=self.cmake.build_dir, quiet=ctx.capture)
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
        assert self.toolchain is self.epics_base.toolchain
        cvars = {
            "EPICS_BASE": self.epics_base.paths["install"],
        }
        if not isinstance(self.toolchain, HostToolchain):
            cvars.update({
                "TOOLCHAIN_DIR": self.toolchain.path,
                "TARGET_TRIPLE": self.toolchain.target,
                "CMAKE_TOOLCHAIN_FILE": os.path.join(self.cmake.src_dir, "armgcc.cmake"),
            })
        self.cmake.configure(ctx, cvars)
        return self.cmake.build(ctx, self.cmake_target)

    def dependencies(self) -> list[Task]:
        deps = list(self.deps)
        if isinstance(self.toolchain, RemoteToolchain):
            deps.append(self.toolchain.download_task)
        deps.append(self.epics_base.build_task)
        return deps

    def artifacts(self) -> str[list]:
        return [self.cmake.build_dir]

class App(Component):
    def __init__(
        self,
        epics_base: EpicsBase,
        toolchain: Toolchain,
    ):
        super().__init__()

        self.epics_base = epics_base
        self.toolchain = toolchain

        self.src_dir = os.path.join(BASE_DIR, "app")
        self.build_dir = os.path.join(TARGET_DIR, f"app_{self.toolchain.name}")

        opts = ["-DCMAKE_BUILD_TYPE=Debug"]
        self.cmake = Cmake(self.src_dir, self.build_dir, opt=opts)

        if isinstance(self.toolchain, HostToolchain):
            self.build_unittest_task = AppBuildUnittestTask(self.cmake)
            self.run_unittest_task = AppRunUnittestTask(self.cmake, self.build_unittest_task)
            self.build_fakedev_task = AppBuildWithEpicsTask(
                self.cmake,
                "app_fakedev",
                self.epics_base,
                self.toolchain,
            )

        self.build_main_task = AppBuildWithEpicsTask(
            self.cmake,
            "app",
            self.epics_base,
            self.toolchain,
        )

    def tasks(self) -> dict[str, Task]:
        tasks = {
            "build_main": self.build_main_task,
        }
        if isinstance(self.toolchain, HostToolchain):
            tasks.update({
                "build_unittest": self.build_unittest_task,
                "run_unittest": self.run_unittest_task,
                "build_fakedev": self.build_fakedev_task,
            })
        return tasks
