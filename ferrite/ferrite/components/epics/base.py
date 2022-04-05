from __future__ import annotations
import json
from typing import Dict, List, ClassVar

import os
import shutil
from pathlib import Path, PurePosixPath
from dataclasses import dataclass, field

import pydantic

from ferrite.utils.run import capture, run
from ferrite.components.base import Artifact, Component, Task, Context
from ferrite.components.toolchain import Target, Toolchain

import logging

logger = logging.getLogger(__name__)


def epics_host_arch(epics_base_dir: Path) -> str:
    return capture([
        "perl",
        epics_base_dir / "src" / "tools" / "EpicsHostArch.pl",
    ])


def epics_arch_by_target(target: Target) -> str:
    if target.api == "linux":
        if target.isa == "x86_64":
            return "linux-x86_64"
        elif target.isa == "arm":
            return "linux-arm"
        elif target.isa == "aarch64":
            return "linux-aarch64"
    # TODO: Add some other archs
    raise Exception(f"Unknown target for EPICS: {str(target)}")


# Milliseconds from the start of the epoch
def _tree_mod_time(path: Path) -> int:
    max_time = 0.0
    for dirpath, dirnames, filenames in os.walk(path):
        max_time = max([
            max_time,
            os.path.getmtime(dirpath),
            *[os.path.getmtime(os.path.join(dirpath, fn)) for fn in filenames],
        ])
    return int(1000 * max_time)


class _BuildInfo(pydantic.BaseModel):
    build_dir: str
    dep_mod_times: Dict[str, int]

    FILE_NAME: ClassVar[str] = "build_info.json"

    @staticmethod
    def from_paths(base_dir: Path, dep_paths: List[Path]) -> _BuildInfo:
        return _BuildInfo(
            build_dir=str(base_dir),
            dep_mod_times={str(path): _tree_mod_time(path) for path in dep_paths},
        )

    @staticmethod
    def load(base_dir: Path) -> _BuildInfo:
        path = base_dir / _BuildInfo.FILE_NAME
        with open(path, "r") as f:
            return _BuildInfo.parse_obj(json.load(f))

    def store(self, base_dir: Path) -> None:
        path = base_dir / _BuildInfo.FILE_NAME
        with open(path, "w") as f:
            json.dump(self.dict(), f, indent=2, sort_keys=True)

    def has_changed_since(self, other: _BuildInfo) -> bool:
        if self.build_dir != other.build_dir:
            return True
        for path, time in self.dep_mod_times.items():
            try:
                if time > other.dep_mod_times[path]:
                    return True
            except KeyError:
                return True
        return False


class AbstractEpicsProject(Component):

    @dataclass
    class BuildTask(Task):
        deps: List[Task]
        clean: bool = False
        cached: bool = False

        def __post_init__(self) -> None:
            self.src_dir = self.owner.src_path
            self.build_dir = self.owner.build_path
            self.install_dir = self.owner.install_path

        @property
        def owner(self) -> AbstractEpicsProject:
            raise NotImplementedError()

        def _prepare_source(self) -> None:
            pass

        def _configure(self) -> None:
            raise NotImplementedError()

        def _install(self) -> None:
            raise NotImplementedError()

        def _dep_paths(self) -> List[Path]:
            return []

        def run(self, ctx: Context) -> None:
            info = _BuildInfo.from_paths(self.build_dir, self._dep_paths())
            try:
                stored_info = _BuildInfo.load(self.build_dir)
            except FileNotFoundError:
                pass
            else:
                if not info.has_changed_since(stored_info):
                    logger.info(f"'{self.build_dir}' is already built")
                    return

            # TODO: Remove after a while
            done_file = self.build_dir / "build.done"
            if done_file.exists():
                logger.info(f"Migrating from 'build.done' to '{_BuildInfo.FILE_NAME}'")
                info.store(self.build_dir)
                os.remove(done_file)
                return

            if self.clean:
                shutil.rmtree(self.build_dir, ignore_errors=True)
                shutil.rmtree(self.install_dir, ignore_errors=True)

            self._prepare_source()

            shutil.copytree(
                self.src_dir,
                self.build_dir,
                dirs_exist_ok=True,
                ignore=shutil.ignore_patterns(".git"),
            )

            logger.info(f"Configure {self.build_dir}")
            self._configure()

            logger.info(f"Build {self.build_dir}")
            run(
                [
                    "make",
                    "--jobs",
                    *([str(ctx.jobs)] if ctx.jobs is not None else []),
                ],
                cwd=self.build_dir,
                quiet=ctx.capture,
            )

            info.store(self.build_dir)

            logger.info(f"Install {self.build_dir} to {self.install_dir}")
            self.install_dir.mkdir(exist_ok=True)
            self._install()

        def dependencies(self) -> List[Task]:
            return self.deps

        def artifacts(self) -> List[Artifact]:
            return [
                Artifact(self.build_dir, cached=self.cached),
                Artifact(self.install_dir, cached=self.cached),
            ]

    @dataclass
    class DeployTask(Task):
        deploy_path: PurePosixPath
        deps: List[Task]
        blacklist: List[str] = field(default_factory=lambda: [])

        @property
        def owner(self) -> AbstractEpicsProject:
            raise NotImplementedError()

        def _pre(self, ctx: Context) -> None:
            pass

        def _post(self, ctx: Context) -> None:
            pass

        def run(self, ctx: Context) -> None:
            assert ctx.device is not None
            self._pre(ctx)
            logger.info(f"Deploy {self.owner.install_path} to {ctx.device.name()}:{self.deploy_path}")
            ctx.device.store(
                self.owner.install_path,
                self.deploy_path,
                recursive=True,
                exclude=self.blacklist,
            )
            self._post(ctx)

        def dependencies(self) -> List[Task]:
            return self.deps

    def __init__(self, target_dir: Path, src_path: Path, prefix: str) -> None:
        super().__init__()

        self.prefix = prefix

        self.src_path = src_path
        self.build_path = target_dir / f"{prefix}_build_{self.toolchain.name}"
        self.install_path = target_dir / f"{prefix}_install_{self.toolchain.name}"

    @property
    def arch(self) -> str:
        return epics_arch_by_target(self.toolchain.target)

    @property
    def toolchain(self) -> Toolchain:
        raise NotImplementedError
