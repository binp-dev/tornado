from __future__ import annotations
from typing import Callable, Dict, List

import shutil
from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Artifact, Component, Task, Context
from ferrite.components.conan import CmakeRunnableWithConan
from ferrite.components.compiler import GccHost, Gcc


class Codegen(Component):

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
        assets_dir: Path,
        target_dir: Path,
        cc: Gcc,
        prefix: str,
        generate: Callable[[Path], None],
    ):
        self.prefix = prefix

        self.assets_dir = assets_dir
        self.gen_dir = target_dir / self.prefix
        self.build_dir = target_dir / f"{self.prefix}_{cc.name}"
        self.test_dir = target_dir / f"{self.prefix}_test"

        self.generate = generate
        self.generate_task = self.GenerateTask(self, self.generate)

    def tasks(self) -> Dict[str, Task]:
        return {
            "generate": self.generate_task,
        }


class CodegenWithTest(Codegen):

    def __init__(
        self,
        assets_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        cc: GccHost,
        prefix: str,
        generate: Callable[[Path], None],
    ):
        super().__init__(
            assets_dir,
            target_dir,
            cc,
            prefix,
            generate,
        )

        self.cmake = CmakeRunnableWithConan(
            self.gen_dir / "test",
            self.test_dir,
            cc,
            target=f"{self.prefix}_test",
            opts=[f"-DFERRITE={ferrite_source_dir}"],
            deps=[self.generate_task],
        )
        self.build_task = self.cmake.build_task
        self.test_task = self.cmake.run_task

    def tasks(self) -> Dict[str, Task]:
        return {
            **super().tasks(),
            "build": self.build_task,
            "test": self.test_task,
        }


class CodegenExample(CodegenWithTest):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        cc: GccHost,
    ):
        from ferrite.codegen.test import generate

        super().__init__(
            source_dir / "codegen",
            source_dir,
            target_dir,
            cc,
            "codegen",
            generate,
        )
