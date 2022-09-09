from __future__ import annotations
from typing import Dict, List

import shutil
from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Artifact, CallTask, Component, Task, Context, TaskList
from ferrite.components.rust import RustcHost, Cargo
from ferrite.components.cmake import Cmake

from ferrite.codegen.base import Context as CodegenContext, TestInfo
from ferrite.codegen.generator import Generator
from ferrite.utils.files import substitute


@dataclass
class Codegen(Component):
    name: str
    source_dir: Path
    output_dir: Path
    generator: Generator
    default_msg: bool # FIXME: Make `True` by default

    @dataclass(eq=False)
    class GenerateTask(Task):
        owner: Codegen

        def run(self, ctx: Context) -> None:
            shutil.copytree(self.owner.assets_dir, self.owner.output_dir, dirs_exist_ok=True)
            for path in [Path("c/CMakeLists.txt"), Path("rust/Cargo.toml"), Path("rust/build.rs")]:
                substitute([("{{codegen}}", self.owner.name)], self.owner.output_dir / path)

            self.owner.generator.generate(self.owner.context).write(self.owner.output_dir)

        def artifacts(self) -> List[Artifact]:
            return [Artifact(self.owner.output_dir)]

    def __post_init__(self) -> None:
        self.assets_dir = self.source_dir / "codegen"
        self.context = CodegenContext(self.name, default=self.default_msg)
        self.generate_task = self.GenerateTask(self)

    def tasks(self) -> Dict[str, Task]:
        return {"generate": self.generate_task}


@dataclass
class CodegenTest(Codegen):
    rustc: RustcHost

    @dataclass(eq=False)
    class GenerateTask(Codegen.GenerateTask):

        def run(self, ctx: Context) -> None:
            super().run(ctx)
            assert isinstance(self.owner, CodegenTest)
            self.owner.generator.generate_tests(self.owner.context, self.owner.test_info).write(self.owner.output_dir)

    def __post_init__(self) -> None:
        super().__post_init__()
        self.assets_dir = self.source_dir / "codegen"
        self.test_info = TestInfo(16)

        self.c_test = Cmake(
            self.output_dir / "c",
            self.output_dir / "c/build",
            self.rustc.cc,
            target=f"{self.name}_test",
            deps=[self.generate_task],
        )
        self.rust_test = Cargo(
            self.output_dir / "rust",
            self.output_dir / "rust/target",
            self.rustc,
            deps=[self.c_test.build_task],
        )
        self.build_task = self.rust_test.build_task
        self.test_task = TaskList([
            self.rust_test.test_task,
            CallTask(lambda: self.generator.self_test(self.test_info)),
        ])

    def tasks(self) -> Dict[str, Task]:
        return {
            **super().tasks(),
            "build": self.build_task,
            "test": self.test_task,
        }
