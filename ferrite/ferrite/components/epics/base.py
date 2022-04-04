from __future__ import annotations
from typing import List

import shutil
from pathlib import Path, PurePosixPath
from dataclasses import dataclass, field

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

        def run(self, ctx: Context) -> None:
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
            run(["make", "--jobs"], cwd=self.build_dir, quiet=ctx.capture)

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
