from __future__ import annotations
from http.server import executable
import shutil
from typing import Callable, Dict, List

from pathlib import Path
from dataclasses import dataclass
from ferrite.components.cmake import CmakeWithTest

from ferrite.utils.run import run
from ferrite.components.base import Artifact, Task, Context
from ferrite.components.conan import CmakeWithConan
from ferrite.components.toolchains import CrossToolchain, HostToolchain, Toolchain


class Codegen(CmakeWithConan, CmakeWithTest):

    @dataclass
    class GenerateTask(Task):

        owner: Codegen
        generate: Callable[[Path], None]

        def run(self, ctx: Context) -> None:
            self.generate(self.owner.gen_dir)
            shutil.copytree(self.owner.assets_dir, self.owner.gen_dir, dirs_exist_ok=True)

        def artifacts(self) -> List[Artifact]:
            return [Artifact(self.owner.gen_dir)]

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
        prefix: str,
        generate: Callable[[Path], None],
    ):
        self.prefix = prefix
        self.executable = f"{self.prefix}_test"

        self.assets_dir = source_dir / self.prefix
        self.gen_dir = target_dir / f"{self.prefix}_generated"
        build_dir = target_dir / f"{self.prefix}_{toolchain.name}"

        self.generate = generate
        self.generate_task = self.GenerateTask(self, self.generate)

        super().__init__(
            self.gen_dir,
            build_dir,
            toolchain,
            opts=[
                "-DCMAKE_BUILD_TYPE=Debug",
                f"-D{self.prefix.upper()}_TEST=1",
            ],
            deps=[self.generate_task],
            target=self.executable,
            disable_conan=isinstance(toolchain, CrossToolchain)
        )

    def tasks(self) -> Dict[str, Task]:
        return {
            "generate": self.generate_task,
            "build": self.build_task,
            "test": self.test_task,
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
