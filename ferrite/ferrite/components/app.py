from __future__ import annotations
from typing import Dict, List

from pathlib import Path

from ferrite.utils.run import run
from ferrite.components.base import Artifact, Component, Task, Context
from ferrite.components.cmake import Cmake
from ferrite.components.conan import CmakeWithConan
from ferrite.components.epics.epics_base import EpicsBase
from ferrite.components.ipp import Ipp
from ferrite.components.toolchains import Toolchain, HostToolchain, CrossToolchain


class AppBuildUnittestTask(Task):

    def __init__(self, cmake: Cmake, ipp: Ipp):
        super().__init__()
        self.cmake = cmake
        self.ipp = ipp

    def run(self, ctx: Context) -> None:
        self.cmake.configure(ctx)
        self.cmake.build(ctx, "app_unittest")

    def dependencies(self) -> List[Task]:
        return [self.ipp.generate_task]

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.cmake.build_dir)]


class AppRunUnittestTask(Task):

    def __init__(self, cmake: Cmake, build_task: Task):
        super().__init__()
        self.cmake = cmake
        self.build_task = build_task

    def run(self, ctx: Context) -> None:
        run(["./app_unittest"], cwd=self.cmake.build_dir, quiet=ctx.capture)

    def dependencies(self) -> List[Task]:
        return [self.build_task]


class AppBuildTask(Task):

    def __init__(
        self,
        cmake: Cmake,
        cmake_target: str,
        ipp: Ipp,
        deps: List[Task] = [],
    ):
        super().__init__()
        self.cmake = cmake
        self.cmake_target = cmake_target
        self.toolchain = cmake.toolchain
        self.ipp = ipp
        self.deps = deps

    def run(self, ctx: Context) -> None:
        self.cmake.configure(ctx)
        self.cmake.build(ctx, self.cmake_target)

    def dependencies(self) -> List[Task]:
        deps = list(self.deps)
        if isinstance(self.cmake.toolchain, CrossToolchain):
            deps.append(self.cmake.toolchain.download_task)
        deps.append(self.ipp.generate_task)
        return deps

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.cmake.build_dir)]


class App(Component):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
        ipp: Ipp,
    ):
        super().__init__()

        self.toolchain = toolchain
        self.ipp = ipp

        self.src_dir = source_dir / "app"
        self.build_dir = target_dir / f"app_{self.toolchain.name}"

        opts = ["-DCMAKE_BUILD_TYPE=Debug", *self.ipp.cmake_opts]
        envs = {}
        if isinstance(self.toolchain, CrossToolchain):
            toolchain_cmake_path = self.src_dir / "armgcc.cmake"
            opts.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_cmake_path}")
            envs.update({
                "TOOLCHAIN_DIR": str(self.toolchain.path),
                "TARGET_TRIPLE": str(self.toolchain.target),
            })
        self.cmake = CmakeWithConan(self.src_dir, self.build_dir, self.toolchain, opt=opts, env=envs)

        if isinstance(self.toolchain, HostToolchain):
            self.build_unittest_task = AppBuildUnittestTask(self.cmake, self.ipp)
            self.run_unittest_task = AppRunUnittestTask(self.cmake, self.build_unittest_task)
            self.build_fakedev_task = AppBuildTask(self.cmake, "app_fakedev", self.ipp)

        self.build_main_task = AppBuildTask(self.cmake, "app", self.ipp)

    def tasks(self) -> Dict[str, Task]:
        tasks: Dict[str, Task] = {
            "build_main": self.build_main_task,
        }
        if isinstance(self.toolchain, HostToolchain):
            tasks.update({
                "build_unittest": self.build_unittest_task,
                "run_unittest": self.run_unittest_task,
                "build_fakedev": self.build_fakedev_task,
            })
        return tasks
