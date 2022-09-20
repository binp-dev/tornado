from __future__ import annotations
from typing import Dict, List

from pathlib import Path
from dataclasses import dataclass, field

from ferrite.utils.path import TargetPath
from ferrite.utils.run import run
from ferrite.components.base import Artifact, Component, OwnedTask, Context, Task
from ferrite.components.compiler import Gcc


@dataclass
class Cmake(Component):

    src_dir: Path | TargetPath
    build_dir: TargetPath
    cc: Gcc
    target: str # TODO: Rename to `cmake_target`
    opts: List[str] = field(default_factory=list)
    envs: Dict[str, str] = field(default_factory=dict)
    deps: List[Task] = field(default_factory=list)

    def __post_init__(self) -> None:
        self.build_task = _BuildTask(self)

    def create_build_dir(self, ctx: Context) -> None:
        (ctx.target_path / self.build_dir).mkdir(exist_ok=True)

    @property
    def _defs(self) -> Dict[str, str]:
        deflist = [opt[2:].split("=") for opt in self.opts if opt.startswith("-D")]
        assert all((len(ds) == 2 for ds in deflist))
        return {k: v for k, v in deflist}

    def configure(self, ctx: Context) -> None:
        self.create_build_dir(ctx)
        run(
            [
                "cmake",
                *self.opts,
                self.src_dir if isinstance(self.src_dir, Path) else ctx.target_path / self.src_dir,
            ],
            cwd=(ctx.target_path / self.build_dir),
            add_env=self.envs,
            quiet=ctx.capture,
        )

    def build(self, ctx: Context, verbose: bool = False) -> None:
        run(
            [
                "cmake",
                "--build",
                ctx.target_path / self.build_dir,
                *["--target", self.target],
                "--parallel",
                *([str(ctx.jobs)] if ctx.jobs is not None else []),
                *(["--verbose"] if verbose else []),
            ],
            cwd=(ctx.target_path / self.build_dir),
            quiet=ctx.capture,
        )


class _BuildTask(OwnedTask[Cmake]):

    def run(self, ctx: Context) -> None:
        self.owner.configure(ctx)
        self.owner.build(ctx)

    def dependencies(self) -> List[Task]:
        return [
            *self.owner.deps,
            self.owner.cc.install_task,
        ]

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.build_dir)]


@dataclass
class CmakeRunnable(Cmake):

    def __post_init__(self) -> None:
        super().__post_init__()
        self.run_task = _RunTask(self)

    def run(self, ctx: Context) -> None:
        run(
            [f"./{self.target}"],
            cwd=(ctx.target_path / self.build_dir),
            quiet=ctx.capture,
        )


class _RunTask(OwnedTask[CmakeRunnable]):

    def run(self, ctx: Context) -> None:
        self.owner.run(ctx)

    def dependencies(self) -> List[Task]:
        return [self.owner.build_task]
