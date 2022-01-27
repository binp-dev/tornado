from __future__ import annotations
from typing import Dict, List

from pathlib import Path

from ferrite.utils.run import run
from ferrite.components.base import Artifact, Component, Task, Context
from ferrite.components.cmake import Cmake, CmakeWithTest
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


class App(CmakeWithConan):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
        ipp: Ipp,
    ):
        src_dir = source_dir / "app"
        build_dir = target_dir / f"app_{toolchain.name}"

        opts: List[str] = [
            "-DCMAKE_BUILD_TYPE=Debug",
            f"-DIPP_GEN_DIR={ipp.gen_dir}",
        ]
        envs: Dict[str, str] = {}
        if isinstance(toolchain, HostToolchain):
            target = "app_fakedev"
            opts.append("-DAPP_FAKEDEV=1")
        if isinstance(toolchain, CrossToolchain):
            target = "app"
            opts.append("-DAPP_MAIN=1")
            toolchain_cmake_path = src_dir / "armgcc.cmake"
            opts.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_cmake_path}")
            envs.update({
                "TOOLCHAIN_DIR": str(toolchain.path),
                "TARGET_TRIPLE": str(toolchain.target),
            })

        super().__init__(
            src_dir,
            build_dir,
            toolchain,
            opts=opts,
            envs=envs,
            deps=[ipp.generate_task],
            target=target,
            disable_conan=isinstance(toolchain, CrossToolchain),
        )
        self.ipp = ipp


class AppTest(CmakeWithConan, CmakeWithTest):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
    ):
        super().__init__(
            source_dir / "app",
            target_dir / "app_test",
            toolchain,
            opts=["-DAPP_TEST=1"],
            target="app_unittest",
        )
