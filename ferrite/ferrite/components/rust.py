from __future__ import annotations
from typing import Dict, List, Any

import re
from pathlib import Path

from ferrite.utils.run import run, capture
from ferrite.components.base import Artifact, Component, OwnedTask, Context, Task
from ferrite.components.compiler import Compiler, Gcc, GccCross, GccHost, Target, _InstallTask as CompilerInstallTask


class Rustc(Compiler):

    def __init__(self, postfix: str, target: Target, target_dir: Path, cc: Gcc):
        self._install_task = _RustcInstallTask(self)
        super().__init__(f"rustc_{postfix}", target, cached=True)
        self.target_dir = target_dir
        self.path = target_dir / "rustup"
        self._cc = cc

    @property
    def install_task(self) -> _RustcInstallTask:
        return self._install_task

    @property
    def cc(self) -> Gcc:
        return self._cc

    def env(self) -> Dict[str, str]:
        return {
            "RUSTUP_HOME": str(self.path),
            "RUSTUP_TOOLCHAIN": "stable",
        }

    def install(self, capture: bool = False) -> None:
        cmds = [
            ["rustup", "set", "profile", "minimal"],
            ["rustup", "target", "add", str(self.target)],
        ]
        for cmd in cmds:
            run(cmd, add_env=self.env(), quiet=capture)


class _RustcInstallTask(CompilerInstallTask, OwnedTask[Rustc]):

    def run(self, ctx: Context) -> None:
        self.owner.install(capture=ctx.capture)

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.path, cached=True)]


class RustcHost(Rustc):

    _target_pattern: re.Pattern[str] = re.compile(r"^Default host:\s+(\S+)$", re.MULTILINE)

    def __init__(self, target_dir: Path, cc: GccHost):
        info = capture(["rustup", "show"])
        match = re.search(self._target_pattern, info)
        assert match is not None, f"Cannot detect rustup host rustc:\n{info}"
        target = Target.from_str(match[1])
        super().__init__("host", target, target_dir, cc)


class RustcCross(Rustc):

    def __init__(self, postfix: str, target: Target, target_dir: Path, cc: GccCross):
        self._cc_cross = cc
        super().__init__(postfix, target, target_dir, cc)

    @property
    def cc(self) -> GccCross:
        return self._cc_cross

    def env(self) -> Dict[str, str]:
        target_uu = str(self.target).upper().replace("-", "_")
        linker = self.cc.bin("gcc")
        return {
            **super().env(),
            f"CARGO_TARGET_{target_uu}_LINKER": str(linker),
        }


class Cargo(Component):

    def __init__(
        self,
        src_dir: Path,
        build_dir: Path,
        rustc: Rustc,
        envs: Dict[str, str] = {},
        deps: List[Task] = [],
    ) -> None:
        super().__init__()

        self.src_dir = src_dir
        self.build_dir = build_dir
        self.rustc = rustc
        self.envs = envs
        self.deps = deps

        self.home_dir = Path.cwd() / ".cargo"

        self.build_task = _CargoBuildTask(self)
        self.test_task = _CargoTestTask(self)

    def _env(self) -> Dict[str, str]:
        return {
            **self.rustc.env(),
            "CARGO_HOME": str(self.home_dir),
            "CARGO_TARGET_DIR": str(self.build_dir),
            **self.envs,
        }

    @property
    def bin_dir(self) -> Path:
        return self.build_dir / str(self.rustc.target) / "debug"

    def build(self, capture: bool = False, update: bool = False) -> None:
        cmds = [
            *([["cargo", "update"]] if update else []),
            ["cargo", "build", f"--target={self.rustc.target}"],
        ]
        for cmd in cmds:
            run(cmd, cwd=self.src_dir, add_env=self._env(), quiet=capture)

    def test(self, capture: bool = False) -> None:
        run(
            ["cargo", "test"],
            cwd=self.src_dir,
            add_env=self._env(),
            quiet=capture,
        )


class _CargoBuildTask(OwnedTask[Cargo]):

    def run(self, ctx: Context) -> None:
        self.owner.build(capture=ctx.capture, update=ctx.update)

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
        self.owner.test(capture=ctx.capture)

    def dependencies(self) -> List[Task]:
        return [
            *super().dependencies(),
            self.owner.build_task,
        ]
