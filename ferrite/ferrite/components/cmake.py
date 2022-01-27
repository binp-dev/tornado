from __future__ import annotations
from typing import Dict, List, Optional

from pathlib import Path
from dataclasses import dataclass, field

from ferrite.utils.run import run
from ferrite.components.base import Artifact, Component, Task, Context
from ferrite.components.toolchains import CrossToolchain, Toolchain


@dataclass
class Cmake(Component):

    @dataclass
    class BuildTask(Task):
        owner: Cmake

        def run(self, ctx: Context) -> None:
            self.owner.configure(ctx)
            self.owner.build(ctx, self.owner.target)

        def dependencies(self) -> List[Task]:
            deps: List[Task] = []
            if isinstance(self.owner.toolchain, CrossToolchain):
                deps.append(self.owner.toolchain.download_task)
            deps.extend(self.owner.deps)
            return deps

        def artifacts(self) -> List[Artifact]:
            return [Artifact(self.owner.build_dir)]

    src_dir: Path
    build_dir: Path
    toolchain: Toolchain
    opts: List[str] = field(default_factory=list)
    envs: Dict[str, str] = field(default_factory=dict)
    deps: List[Task] = field(default_factory=list)
    target: Optional[str] = None

    def __post_init__(self) -> None:
        self.build_task = self.BuildTask(self)

    def create_build_dir(self) -> None:
        self.build_dir.mkdir(exist_ok=True)

    def configure(self, ctx: Context) -> None:
        self.create_build_dir()
        run(
            [
                "cmake",
                *self.opts,
                self.src_dir,
            ],
            cwd=self.build_dir,
            add_env=self.envs,
            quiet=ctx.capture,
        )

    def build(self, ctx: Context, target: Optional[str] = None, verbose: bool = False) -> None:
        if target is None:
            target = self.target
        run(
            [
                "cmake",
                "--build",
                self.build_dir,
                *(["--target", target] if target else []),
                "--parallel",
                *(["--verbose"] if verbose else []),
            ],
            cwd=self.build_dir,
            quiet=ctx.capture,
        )

    def tasks(self) -> Dict[str, Task]:
        return {"build": self.build_task}


@dataclass
class CmakeWithTest(Cmake):

    @dataclass
    class TestTask(Task):
        owner: CmakeWithTest

        def run(self, ctx: Context) -> None:
            self.owner.test(ctx)

        def dependencies(self) -> List[Task]:
            return [self.owner.build_task]

    def __post_init__(self) -> None:
        super().__post_init__()
        self.test_task = self.TestTask(self)

    def test(self, ctx: Context) -> None:
        run(
            [f"./{self.target}"] if self.target else ["ctest", "--verbose"],
            cwd=self.build_dir,
            quiet=ctx.capture,
        )

    def tasks(self) -> Dict[str, Task]:
        tasks = super().tasks()
        tasks["test"] = self.test_task
        return tasks
