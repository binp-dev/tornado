from __future__ import annotations
from typing import Dict, List

import os

from ferrite.utils.run import run
from ferrite.manage.paths import BASE_DIR, TARGET_DIR
from ferrite.components.base import Component, Task, Context
from ferrite.components.cmake import Cmake
from ferrite.components.conan import CmakeWithConan
from ferrite.components.toolchains import Toolchain
from ferrite.components.codegen import Codegen


class IppBuildUnittestTask(Task):

    def __init__(self, cmake: Cmake, generate_task: Task):
        super().__init__()
        self.cmake = cmake
        self.generate_task = generate_task

    def run(self, ctx: Context) -> None:
        self.cmake.configure(ctx)
        self.cmake.build(ctx, "ipp_test")

    def artifacts(self) -> List[str]:
        return [self.cmake.build_dir]

    def dependencies(self) -> List[Task]:
        return [self.generate_task]


class IppRunUnittestTask(Task):

    def __init__(self, cmake: Cmake, build_task: Task):
        super().__init__()
        self.cmake = cmake
        self.build_task = build_task

    def run(self, ctx: Context) -> None:
        run(["./ipp_test"], cwd=self.cmake.build_dir, quiet=ctx.capture)

    def dependencies(self) -> List[Task]:
        return [self.build_task]


class IppGenerate(Task):

    def __init__(self, owner: Ipp):
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> None:
        from ferrite.ipp import generate
        generate(self.owner.generated_dir)

    def artifacts(self) -> List[str]:
        return [self.owner.generated_dir]


class Ipp(Component):

    def __init__(
        self,
        toolchain: Toolchain,
        codegen: Codegen,
    ):
        super().__init__()

        self.codegen = codegen

        self.src_dir = os.path.join(BASE_DIR, "ipp")
        self.generated_dir = os.path.join(TARGET_DIR, f"ipp_generated")
        self.build_dir = os.path.join(TARGET_DIR, f"ipp_{toolchain.name}")

        self.cmake_opts = [f"-DIPP_GENERATED={self.generated_dir}"]
        self.test_cmake = CmakeWithConan(
            self.src_dir,
            self.build_dir,
            toolchain,
            opt=[
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DIPP_UNITTEST=1",
                *self.cmake_opts,
            ],
        )

        self.generate_task = IppGenerate(self)
        self.build_unittest_task = IppBuildUnittestTask(self.test_cmake, self.generate_task)
        self.run_unittest_task = IppRunUnittestTask(self.test_cmake, self.build_unittest_task)

    def tasks(self) -> Dict[str, Task]:
        return {
            "generate": self.generate_task,
            "build": self.build_unittest_task,
            "test": self.run_unittest_task,
        }
