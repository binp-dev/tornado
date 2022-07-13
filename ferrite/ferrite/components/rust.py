from __future__ import annotations
from json import tool
from typing import Dict, List, Optional

import re
from pathlib import Path
from dataclasses import dataclass, field

from ferrite.utils.run import run, capture
from ferrite.components.base import Artifact, Component, Task, Context
from ferrite.components.toolchain import Target, Toolchain


class Rustup(Toolchain):

    class InstallTask(Task):

        def __init__(self, owner: Rustup) -> None:
            super().__init__()
            self.owner = owner

        def run(self, ctx: Context) -> None:
            self.owner.install(capture=ctx.capture)

        def artifacts(self) -> List[Artifact]:
            return [Artifact(self.owner.path, cached=True)]

    def __init__(self, postfix: str, target: Target, target_dir: Path):
        super().__init__(f"rustup_{postfix}", target, cached=True)
        self.target_dir = target_dir
        self.path = target_dir / "rustup"
        self.install_task = self.InstallTask(self)

    def env(self) -> Dict[str, str]:
        return {
            "RUSTUP_HOME": str(self.path),
            "RUSTUP_TOOLCHAIN": "stable",
        }

    def install(self, capture: bool = False) -> bool:
        run(
            ["rustup", "target", "add", str(self.target)],
            add_env=self.env(),
            quiet=capture,
        )
        return True

    def tasks(self) -> Dict[str, Task]:
        return {"install": self.install_task}


class HostRustup(Rustup):

    _target_pattern: re.Pattern[str] = re.compile(r"^Default host:\s+(\S+)$", re.MULTILINE)

    def __init__(self, target_dir: Path):
        info = capture(["rustup", "show"])
        match = re.search(self._target_pattern, info)
        assert match is not None, f"Cannot detect rustup host toolchain:\n{info}"
        target = Target.from_str(match[1])
        super().__init__("host", target, target_dir)


class Cargo(Component):

    @dataclass
    class BuildTask(Task):
        owner: Cargo

        def run(self, ctx: Context) -> None:
            self.owner.build(capture=ctx.capture)

        def dependencies(self) -> List[Task]:
            return self.owner.deps + [self.owner.toolchain.install_task]

        def artifacts(self) -> List[Artifact]:
            return [Artifact(self.owner.build_dir)]

    def __init__(
        self,
        src_dir: Path,
        build_dir: Path,
        toolchain: Rustup,
        deps: List[Task] = [],
    ) -> None:
        super().__init__()

        self.src_dir = src_dir
        self.build_dir = build_dir
        self.toolchain = toolchain
        self.deps = deps

        self.home_dir = self.toolchain.target_dir / "cargo"

        self.build_task = self.BuildTask(self)

    def _env(self) -> Dict[str, str]:
        return {
            **self.toolchain.env(),
            "CARGO_HOME": str(self.home_dir),
            "CARGO_TARGET_DIR": str(self.build_dir),
        }

    def build(self, capture: bool = False) -> None:
        run(
            ["cargo", "build", f"--target={self.toolchain.target}"],
            cwd=self.src_dir,
            add_env=self._env(),
            quiet=capture,
        )

    def tasks(self) -> Dict[str, Task]:
        return {"build": self.build_task}


class CargoWithTest(Cargo):

    @dataclass
    class TestTask(Task):
        owner: CargoWithTest

        def run(self, ctx: Context) -> None:
            self.owner.test(capture=ctx.capture)

        def dependencies(self) -> List[Task]:
            return self.owner.deps + [self.owner.build_task, self.owner.toolchain.install_task]

    def __init__(
        self,
        src_dir: Path,
        build_dir: Path,
        toolchain: HostRustup,
        deps: List[Task] = [],
    ) -> None:
        super().__init__(src_dir, build_dir, toolchain, deps=deps)

        self.test_task = self.TestTask(self)

    def test(self, capture: bool = False) -> None:
        run(
            ["cargo", "test"],
            cwd=self.src_dir,
            add_env=self._env(),
            quiet=capture,
        )

    def tasks(self) -> Dict[str, Task]:
        return {
            **super().tasks(),
            "test": self.test_task,
        }
