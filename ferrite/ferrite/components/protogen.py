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
from ferrite.protogen.base import Context as GenContext, TestInfo
from ferrite.protogen.configen import Config
from ferrite.protogen.generator import Generator
from ferrite.info import path as self_path


@dataclass
class _Generator(Component):

    name: str
    output_dir: TargetPath

    def __post_init__(self) -> None:
        self.generate_task = self.GenerateTask()

    def context(self) -> GenContext:
        raise NotImplementedError()

    def GenerateTask(self) -> _GenerateTask[_Generator]:
        raise NotImplementedError()


O = TypeVar("O", bound=_Generator, covariant=True)


class _GenerateTask(OwnedTask[O]):

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.output_dir)]


@dataclass
class Configen(_Generator):

    config: Config

    def context(self) -> GenContext:
        return GenContext(self.name)

    def GenerateTask(self) -> _ConfigenTask:
        return _ConfigenTask(self)


class _ConfigenTask(_GenerateTask[Configen]):

    def run(self, ctx: Context) -> None:
        output_path = ctx.target_path / self.owner.output_dir
        self.owner.config.generate(self.owner.context()).write(output_path)


@dataclass
class Protogen(_Generator):

    generator: Generator
    default_msg: bool # FIXME: Make `True` by default

    def __post_init__(self) -> None:
        self.assets_path = self_path / "source/protogen"
        self.generate_task = self.GenerateTask()

    def context(self) -> GenContext:
        return GenContext(self.name, default=self.default_msg)

    def GenerateTask(self) -> _ProtogenTask[Protogen]:
        return _ProtogenTask(self)


Op = TypeVar("Op", bound=Protogen, covariant=True)


class _ProtogenTask(_GenerateTask[Op]):

    def run(self, ctx: Context) -> None:
        output_path = ctx.target_path / self.owner.output_dir

        shutil.copytree(self.owner.assets_path, output_path, dirs_exist_ok=True)
        for path in [Path("c/CMakeLists.txt"), Path("rust/Cargo.toml"), Path("rust/build.rs")]:
            substitute([("{{protogen}}", self.owner.name)], output_path / path)

        self.owner.generator.generate(self.owner.context()).write(output_path)


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


class _GenerateTestTask(_ProtogenTask[ProtogenTest]):

    def run(self, ctx: Context) -> None:
        super().run(ctx)
        self.owner.generator.generate_tests(
            self.owner.context(),
            self.owner.test_info,
        ).write(ctx.target_path / self.owner.output_dir)
