from __future__ import annotations
from typing import ClassVar, Dict, List, Type, TypeVar

import shutil
from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Artifact, CallTask, Component, OwnedTask, Context, TaskList
from ferrite.components.rust import RustcHost, Cargo
from ferrite.components.cmake import Cmake

from ferrite.protogen.base import Context as CodegenContext, TestInfo
from ferrite.protogen.generator import Generator
from ferrite.utils.files import substitute


@dataclass
class Protogen(Component):

    name: str
    source_dir: Path
    output_dir: Path
    generator: Generator
    default_msg: bool # FIXME: Make `True` by default

    def __post_init__(self) -> None:
        self.assets_dir = self.source_dir / "protogen"
        self.context = CodegenContext(self.name, default=self.default_msg)
        self.generate_task = self.GenerateTask()

    def GenerateTask(self) -> _GenerateTask[Protogen]:
        return _GenerateTask(self)


O = TypeVar("O", bound=Protogen, covariant=True)


class _GenerateTask(OwnedTask[O]):

    def run(self, ctx: Context) -> None:
        shutil.copytree(self.owner.assets_dir, self.owner.output_dir, dirs_exist_ok=True)
        for path in [Path("c/CMakeLists.txt"), Path("rust/Cargo.toml"), Path("rust/build.rs")]:
            substitute([("{{protogen}}", self.owner.name)], self.owner.output_dir / path)

        self.owner.generator.generate(self.owner.context).write(self.owner.output_dir)

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.output_dir)]


@dataclass
class ProtogenTest(Protogen):
    rustc: RustcHost

    def __post_init__(self) -> None:
        super().__post_init__()
        self.assets_dir = self.source_dir / "protogen"
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
            deps=[self.generate_task, self.c_test.build_task],
        )
        self.build_task = self.rust_test.build_task
        self.test_task = TaskList([
            self.rust_test.test_task,
            CallTask(lambda: self.generator.self_test(self.test_info)),
        ])

    def GenerateTask(self) -> _GenerateTestTask:
        return _GenerateTestTask(self)


class _GenerateTestTask(_GenerateTask[ProtogenTest]):

    def run(self, ctx: Context) -> None:
        super().run(ctx)
        self.owner.generator.generate_tests(self.owner.context, self.owner.test_info).write(self.owner.output_dir)
