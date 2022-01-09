from __future__ import annotations
import os
from ferrite.utils.run import run
from ferrite.manage.paths import BASE_DIR, TARGET_DIR
from ferrite.components.base import Component, Task, Context
from ferrite.components.cmake import Cmake
from ferrite.components.conan import CmakeWithConan
from ferrite.components.toolchains import HostToolchain


class CodegenBuildTestTask(Task):

    def __init__(self, cmake: Cmake, generate_task: Task):
        super().__init__()
        self.cmake = cmake
        self.generate_task = generate_task

    def run(self, ctx: Context) -> bool:
        self.cmake.configure(ctx)
        return self.cmake.build(ctx, "codegen_test")

    def artifacts(self) -> str[list]:
        return [self.cmake.build_dir]

    def dependencies(self) -> list[Task]:
        return [self.generate_task]


class CodegenRunTestTask(Task):

    def __init__(self, cmake: Cmake, build_task: Task):
        super().__init__()
        self.cmake = cmake
        self.build_task = build_task

    def run(self, ctx: Context) -> bool:
        run(["./codegen_test"], cwd=self.cmake.build_dir, quiet=ctx.capture)
        return True

    def dependencies(self) -> list[Task]:
        return [self.build_task]


class CodegenGenerateTestTask(Task):

    def __init__(self, owner):
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> bool:
        from ferrite.codegen.test import generate
        generate(self.owner.generated_dir)
        return True

    def artifacts(self) -> str[list]:
        return [self.owner.generated_dir]


class Codegen(Component):

    def __init__(
        self,
        toolchain: HostToolchain,
    ):
        super().__init__()

        assert isinstance(toolchain, HostToolchain)
        self.toolchain = toolchain

        self.src_dir = os.path.join(BASE_DIR, "codegen")
        self.generated_dir = os.path.join(TARGET_DIR, f"codegen_test_src")
        self.build_dir = os.path.join(TARGET_DIR, f"codegen_{self.toolchain.name}")

        self.cmake = CmakeWithConan(self.src_dir, self.build_dir, self.toolchain, [f"-DCODEGEN_TEST={self.generated_dir}"])

        self.generate_task = CodegenGenerateTestTask(self)
        self.build_test_task = CodegenBuildTestTask(self.cmake, self.generate_task)
        self.run_test_task = CodegenRunTestTask(self.cmake, self.build_test_task)

    def tasks(self) -> dict[str, Task]:
        return {
            "generate": self.generate_task,
            "build": self.build_test_task,
            "test": self.run_test_task,
        }
