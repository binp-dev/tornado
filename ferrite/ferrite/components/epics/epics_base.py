from __future__ import annotations
from typing import Dict, List, TypeVar

import shutil
from pathlib import Path, PurePosixPath
from dataclasses import dataclass

from ferrite.utils.files import substitute, allow_patterns
from ferrite.components.base import Task
from ferrite.components.git import RepoList, RepoSource
from ferrite.components.compiler import Gcc, GccHost, GccCross
from ferrite.components.epics.base import EpicsProject, EpicsBuildTask, EpicsDeployTask, C

import logging

logger = logging.getLogger(__name__)


class AbstractEpicsBase(EpicsProject[C]):

    def __init__(self, target_dir: Path, src_path: Path, cc: C) -> None:
        super().__init__(target_dir, src_path, cc.name)
        self._cc = cc

    @property
    def cc(self) -> C:
        return self._cc

    @property
    def build_task(self) -> _BuildTask[AbstractEpicsBase[C]]:
        raise NotImplementedError()


class EpicsBaseHost(AbstractEpicsBase[GccHost]):

    def __init__(self, target_dir: Path, cc: GccHost):

        self.version = "7.0.6.1"
        name = f"epics_base_{self.version}"

        super().__init__(
            target_dir / name,
            target_dir / name / "src",
            cc,
        )

        self.name = name
        self.repo = RepoList(
            self.src_path,
            [
                RepoSource("https://gitlab.inp.nsk.su/epics/epics-base.git", f"binp-R{self.version}"),
                RepoSource("https://github.com/epics-base/epics-base.git", f"R{self.version}"),
            ],
            cached=True,
        )

        self._build_task = _HostBuildTask(self)

    @property
    def build_task(self) -> _HostBuildTask:
        return self._build_task


class EpicsBaseCross(AbstractEpicsBase[GccCross]):

    def __init__(self, target_dir: Path, cc: GccCross, host_base: EpicsBaseHost):

        super().__init__(target_dir / host_base.name, host_base.build_path, cc)

        self.host_base = host_base

        self.deploy_path = PurePosixPath("/opt/epics_base")

        self._build_task = _CrossBuildTask(self)
        self.deploy_task = _CrossDeployTask(self, self.deploy_path)

    @property
    def build_task(self) -> _CrossBuildTask:
        return self._build_task


O = TypeVar("O", bound=AbstractEpicsBase[Gcc], covariant=True)


class _BuildTask(EpicsBuildTask[O]):

    def __init__(self, owner: O) -> None:
        super().__init__(owner, clean=False, cached=True)

    def _configure_common(self) -> None:
        defs = [
            #("USR_CFLAGS", ""),
            #("USR_CPPFLAGS", ""),
            ("USR_CXXFLAGS", "-std=c++20"),
            ("BIN_PERMISSIONS", "755"),
            ("LIB_PERMISSIONS", "644"),
            ("SHRLIB_PERMISSIONS", "755"),
            ("INSTALL_PERMISSIONS", "644"),
        ]
        rules = [(f"^(\\s*{k}\\s*=).*$", f"\\1 {v}") for k, v in defs]
        logger.info(rules)
        substitute(
            rules,
            self.build_dir / "configure/CONFIG_COMMON",
        )

    def _configure_toolchain(self) -> None:
        raise NotImplementedError()

    def _configure_install(self) -> None:
        substitute([
            ("^\\s*#*(\\s*INSTALL_LOCATION\\s*=).*$", f"\\1 {self.install_dir}"),
        ], self.build_dir / "configure/CONFIG_SITE")

    def _configure(self) -> None:
        self._configure_common()
        self._configure_toolchain()
        #self._configure_install() # Install is broken

    # Workaround for broken EPICS install
    def _install(self) -> None:
        # Copy all required dirs manually
        paths = [
            "bin",
            "cfg",
            #"configure",
            "db",
            "dbd",
            #"html",
            "include",
            "lib",
            #"templates",
        ]
        for path in paths:
            shutil.copytree(
                self.build_dir / path,
                self.install_dir / path,
                dirs_exist_ok=True,
                symlinks=True,
                ignore=shutil.ignore_patterns("O.*"),
            )


class _HostBuildTask(_BuildTask[EpicsBaseHost]):

    def _configure_toolchain(self) -> None:
        substitute(
            [("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", "\\1")],
            self.build_dir / "configure/CONFIG_SITE",
        )

    def dependencies(self) -> List[Task]:
        return [
            *super().dependencies(),
            self.owner.repo.clone_task,
        ]


class _CrossBuildTask(_BuildTask[EpicsBaseCross]):

    def _configure_toolchain(self) -> None:
        cc = self.owner.cc

        host_arch = self.owner.host_base.arch
        cross_arch = self.owner.arch
        assert cross_arch != host_arch

        if cross_arch == "linux-arm" and host_arch.endswith("-x86_64"):
            host_arch = host_arch[:-3] # Trim '_64'

        substitute(
            [("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {cross_arch}")],
            self.build_dir / "configure/CONFIG_SITE",
        )
        substitute(
            [
                ("^(\\s*GNU_TARGET\\s*=).*$", f"\\1 {str(cc.target)}"),
                ("^(\\s*GNU_DIR\\s*=).*$", f"\\1 {cc.path}"),
            ],
            self.build_dir / f"configure/os/CONFIG_SITE.{host_arch}.{cross_arch}",
        )

    def _install(self) -> None:
        super()._install()

        host_arch = self.owner.host_base.arch
        cross_arch = self.owner.arch
        assert cross_arch != host_arch

        shutil.copytree(
            self.build_dir / "bin" / host_arch,
            self.install_dir / "bin" / cross_arch,
            dirs_exist_ok=True,
            symlinks=True,
            ignore=allow_patterns("*.pl", "*.py"),
        )

    def dependencies(self) -> List[Task]:
        return [
            *super().dependencies(),
            self.owner.host_base.build_task,
        ]


class _CrossDeployTask(EpicsDeployTask[EpicsBaseCross]):

    def __init__(self, owner: EpicsBaseCross, deploy_path: PurePosixPath):
        super().__init__(
            owner,
            deploy_path,
            blacklist=[
                "*.a",
                "include/*",
                f"*/{owner.host_base.arch}/*",
            ],
        )

    def dependencies(self) -> List[Task]:
        return [
            *super().dependencies(),
            self.owner.build_task,
        ]
