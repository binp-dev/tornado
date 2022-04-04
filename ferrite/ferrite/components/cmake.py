from __future__ import annotations
from typing import Dict, List, Optional

from pathlib import Path
from dataclasses import dataclass, field

from ferrite.utils.run import run
from ferrite.components.base import Artifact, Component, Task, Context
from ferrite.components.toolchain import CrossToolchain, Toolchain


@dataclass
class Cmake(Component):

    @dataclass
    class BuildTask(Task):
        owner: Cmake

        def run(self, ctx: Context) -> None:
            self.owner.configure(capture=ctx.capture)
            self.owner.build(jobs=ctx.jobs, capture=ctx.capture)

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
    target: str # FIXME: Rename to `cmake_target`
    opts: List[str] = field(default_factory=list)
    envs: Dict[str, str] = field(default_factory=dict)
    deps: List[Task] = field(default_factory=list)

    def __post_init__(self) -> None:
        self.build_task = self.BuildTask(self)

    def create_build_dir(self) -> None:
        self.build_dir.mkdir(exist_ok=True)

    @property
    def _defs(self) -> Dict[str, str]:
        deflist = [opt[2:].split("=") for opt in self.opts if opt.startswith("-D")]
        assert all((len(ds) == 2 for ds in deflist))
        return {k: v for k, v in deflist}

    def configure(self, capture: bool = False) -> None:
        self.create_build_dir()
        run(
            [
                "cmake",
                *self.opts,
                self.src_dir,
            ],
            cwd=self.build_dir,
            add_env=self.envs,
            quiet=capture,
        )

    def build(self, jobs: Optional[int] = None, capture: bool = False, verbose: bool = False) -> None:
        run(
            [
                "cmake",
                "--build",
                self.build_dir,
                *["--target", self.target],
                "--parallel",
                *([str(jobs)] if jobs is not None else []),
                *(["--verbose"] if verbose else []),
            ],
            cwd=self.build_dir,
            quiet=capture,
        )

    def tasks(self) -> Dict[str, Task]:
        return {"build": self.build_task}


@dataclass
class CmakeRunnable(Cmake):

    @dataclass
    class RunTask(Task):
        owner: CmakeRunnable

        def run(self, ctx: Context) -> None:
            self.owner.run(ctx)

        def dependencies(self) -> List[Task]:
            return [self.owner.build_task]

    def __post_init__(self) -> None:
        super().__post_init__()
        self.run_task = self.RunTask(self)

    def run(self, ctx: Context) -> None:
        run(
            [f"./{self.target}"],
            cwd=self.build_dir,
            quiet=ctx.capture,
        )

    def tasks(self) -> Dict[str, Task]:
        tasks = super().tasks()
        tasks["run"] = self.run_task
        return tasks
