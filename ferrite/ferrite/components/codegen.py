from __future__ import annotations
from typing import Callable, Dict, List

from pathlib import Path
from dataclasses import dataclass

from ferrite.utils.run import run
from ferrite.components.base import Artifact, Component, Task, Context
from ferrite.components.cmake import Cmake
from ferrite.components.conan import CmakeWithConan
from ferrite.components.toolchains import HostToolchain, Toolchain


@dataclass
class CodegenBuildTestTask(Task):

    executable: str
    cmake: Cmake
    generate_task: Task

    def run(self, ctx: Context) -> None:
        self.cmake.configure(ctx)
        self.cmake.build(ctx, self.executable)

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.cmake.build_dir)]

    def dependencies(self) -> List[Task]:
        return [self.generate_task]


@dataclass
class CodegenRunTestTask(Task):
    executable: str
    cmake: Cmake
    build_task: Task

    def run(self, ctx: Context) -> None:
        run([f"./{self.executable}"], cwd=self.cmake.build_dir, quiet=ctx.capture)

    def dependencies(self) -> List[Task]:
        return [self.build_task]


@dataclass
class CodegenGenerateTestTask(Task):

    owner: Codegen
    generate: Callable[[Path], None]

    def run(self, ctx: Context) -> None:
        self.generate(self.owner.generated_dir)

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.generated_dir)]


class Codegen(Component):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
        prefix: str,
        generate: Callable[[Path], None],
    ):
        super().__init__()

        self.toolchain = toolchain
        self.prefix = prefix
        self.generate = generate

        self.src_dir = source_dir / self.prefix
        self.generated_dir = target_dir / f"{self.prefix}_generated"
        self.build_dir = target_dir / f"{self.prefix}_{self.toolchain.name}"

        self.cmake_opts = [f"-D{self.prefix.upper()}_GENERATED={self.generated_dir}"]
        self.cmake = CmakeWithConan(
            self.src_dir,
            self.build_dir,
            self.toolchain,
            opt=[
                "-DCMAKE_BUILD_TYPE=Debug",
                f"-D{self.prefix.upper()}_TEST=1",
                *self.cmake_opts,
            ],
        )

        self.executable = f"{self.prefix}_test"
        self.generate_task = CodegenGenerateTestTask(self, self.generate)
        self.build_test_task = CodegenBuildTestTask(self.executable, self.cmake, self.generate_task)
        self.run_test_task = CodegenRunTestTask(self.executable, self.cmake, self.build_test_task)

    def tasks(self) -> Dict[str, Task]:
        return {
            "generate": self.generate_task,
            "build": self.build_test_task,
            "test": self.run_test_task,
        }


class CodegenTest(Codegen):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
    ):
        from ferrite.codegen.test import generate

        super().__init__(source_dir, target_dir, toolchain, "codegen", generate)
