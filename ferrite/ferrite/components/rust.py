from __future__ import annotations
from typing import Dict, List, Any

import re
from pathlib import Path
from dataclasses import dataclass, field

from ferrite.utils.path import TargetPath
from ferrite.utils.run import run, capture
from ferrite.components.base import Artifact, Component, OwnedTask, Context, Task
from ferrite.components.compiler import Compiler, Gcc, GccCross, GccHost, Target, _InstallTask as CompilerInstallTask


class Rustc(Compiler):

    def __init__(self, postfix: str, target: Target, cc: Gcc):
        self._install_task = _RustcInstallTask(self)
        super().__init__(f"rustc_{postfix}", target, cached=True)
        self.path = TargetPath("rustup")
        self._cc = cc

    @property
    def install_task(self) -> _RustcInstallTask:
        return self._install_task

    @property
    def cc(self) -> Gcc:
        return self._cc

    def env(self, ctx: Context) -> Dict[str, str]:
        return {
            "RUSTUP_HOME": str(ctx.target_path / self.path),
            "RUSTUP_TOOLCHAIN": "stable",
        }

    def install(self, ctx: Context) -> None:
        cmds = [
            ["rustup", "set", "profile", "minimal"],
            ["rustup", "target", "add", str(self.target)],
        ]
        for cmd in cmds:
            run(cmd, add_env=self.env(ctx), quiet=ctx.capture)


class _RustcInstallTask(CompilerInstallTask, OwnedTask[Rustc]):

    def run(self, ctx: Context) -> None:
        self.owner.install(ctx)

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.path, cached=True)]


class RustcHost(Rustc):

    _target_pattern: re.Pattern[str] = re.compile(r"^Default host:\s+(\S+)$", re.MULTILINE)

    def __init__(self, cc: GccHost):
        info = capture(["rustup", "show"])
        match = re.search(self._target_pattern, info)
        assert match is not None, f"Cannot detect rustup host rustc:\n{info}"
        target = Target.from_str(match[1])
        super().__init__("host", target, cc)


class RustcCross(Rustc):

    def __init__(self, postfix: str, target: Target, cc: GccCross):
        self._cc_cross = cc
        super().__init__(postfix, target, cc)

    @property
    def cc(self) -> GccCross:
        return self._cc_cross

    def env(self, ctx: Context) -> Dict[str, str]:
        target_uu = str(self.target).upper().replace("-", "_")
        linker = ctx.target_path / self.cc.bin("gcc")
        return {
            **super().env(ctx),
            f"CARGO_TARGET_{target_uu}_LINKER": str(linker),
        }


@dataclass
class Cargo(Component):
    src_dir: Path | TargetPath
    build_dir: TargetPath
    rustc: Rustc
    deps: List[Task] = field(default_factory=list)
    features: List[str] = field(default_factory=list)

    def __post_init__(self) -> None:
        self.home_dir = Path.cwd() / ".cargo"

        self.build_task = _CargoBuildTask(self)
        self.test_task = _CargoTestTask(self)

    def env(self, ctx: Context) -> Dict[str, str]:
        return {
            **self.rustc.env(ctx),
            "CARGO_HOME": str(self.home_dir),
            "CARGO_TARGET_DIR": str(ctx.target_path / self.build_dir),
        }

    @property
    def bin_dir(self) -> TargetPath:
        return self.build_dir / str(self.rustc.target) / "debug"

    def src_path(self, ctx: Context) -> Path:
        if isinstance(self.src_dir, Path):
            return self.src_dir
        else:
            return ctx.target_path / self.src_dir

    def build(self, ctx: Context) -> None:
        cmds = [
            *([["cargo", "update"]] if ctx.update else []),
            [
                "cargo",
                "build",
                f"--target={self.rustc.target}",
                f"--features={','.join(self.features)}",
            ],
        ]
        for cmd in cmds:
            run(cmd, cwd=self.src_path(ctx), add_env=self.env(ctx), quiet=ctx.capture)

    def test(self, ctx: Context) -> None:
        run(
            ["cargo", "test", f"--features={','.join(self.features)}"],
            cwd=self.src_path(ctx),
            add_env=self.env(ctx),
            quiet=ctx.capture,
        )


class _CargoBuildTask(OwnedTask[Cargo]):

    def run(self, ctx: Context) -> None:
        self.owner.build(ctx)

    def dependencies(self) -> List[Task]:
        return [
            *self.owner.deps,
            self.owner.rustc.install_task,
            self.owner.rustc.cc.install_task,
        ]

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.build_dir)]


class _CargoTestTask(_CargoBuildTask):

    def run(self, ctx: Context) -> None:
        self.owner.test(ctx)

    def dependencies(self) -> List[Task]:
        return [
            *super().dependencies(),
            self.owner.build_task,
        ]
