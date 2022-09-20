from __future__ import annotations
from typing import List, TypeVar

import shutil
from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Artifact, CallTask, Component, OwnedTask, Context, TaskList
from ferrite.components.rust import RustcHost, Cargo
from ferrite.components.cmake import Cmake

from ferrite.utils.path import TargetPath
from ferrite.utils.files import substitute
from ferrite.protogen.base import Context as ProtogenContext, TestInfo
from ferrite.protogen.generator import Generator
from ferrite.info import path as self_path


@dataclass
class Protogen(Component):

    name: str
    output_dir: TargetPath
    generator: Generator
    default_msg: bool # FIXME: Make `True` by default

    def __post_init__(self) -> None:
        self.assets_path = self_path / "source/protogen"
        self.context = ProtogenContext(self.name, default=self.default_msg)
        self.generate_task = self.GenerateTask()

    def GenerateTask(self) -> _GenerateTask[Protogen]:
        return _GenerateTask(self)


O = TypeVar("O", bound=Protogen, covariant=True)


class _GenerateTask(OwnedTask[O]):

    def run(self, ctx: Context) -> None:
        output_path = ctx.target_path / self.owner.output_dir

        shutil.copytree(self.owner.assets_path, output_path, dirs_exist_ok=True)
        for path in [Path("c/CMakeLists.txt"), Path("rust/Cargo.toml"), Path("rust/build.rs")]:
            substitute([("{{protogen}}", self.owner.name)], output_path / path)

        self.owner.generator.generate(self.owner.context).write(output_path)

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.output_dir)]


@dataclass
class ProtogenTest(Protogen):
    rustc: RustcHost

    def __post_init__(self) -> None:
        super().__post_init__()
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
        self.owner.generator.generate_tests(
            self.owner.context,
            self.owner.test_info,
        ).write(ctx.target_path / self.owner.output_dir)
