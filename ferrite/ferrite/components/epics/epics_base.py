from __future__ import annotations
from typing import Dict, List

import shutil
from pathlib import Path, PurePosixPath

from ferrite.utils.files import substitute, allow_patterns
from ferrite.components.base import Task
from ferrite.components.git import RepoList, RepoSource
from ferrite.components.compiler import GccHost, GccCross
from ferrite.components.epics.base import AbstractEpicsProject

import logging

logger = logging.getLogger(__name__)


class AbstractEpicsBase(AbstractEpicsProject):

    class BuildTask(AbstractEpicsProject.BuildTask):

        def __init__(self) -> None:
            super().__init__(cached=True)

        @property
        def owner(self) -> AbstractEpicsBase:
            raise NotImplementedError()

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

    @property
    def build_task(self) -> AbstractEpicsBase.BuildTask:
        raise NotImplementedError()


class EpicsBaseHost(AbstractEpicsBase):

    class BuildTask(AbstractEpicsBase.BuildTask):

        def __init__(self, owner: EpicsBaseHost):
            self._owner = owner
            super().__init__()

        @property
        def owner(self) -> EpicsBaseHost:
            return self._owner

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

    def __init__(self, target_dir: Path, cc: GccHost):

        self.version = "7.0.6.1"
        name = f"epics_base_{self.version}"

        self._cc = cc
        super().__init__(
            target_dir / name,
            target_dir / name / "src",
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

        self._build_task = self.BuildTask(self)

    @property
    def build_task(self) -> BuildTask:
        return self._build_task

    @property
    def cc(self) -> GccHost:
        return self._cc

    def tasks(self) -> Dict[str, Task]:
        return {
            "clone": self.repo.clone_task,
            "build": self.build_task,
        }


class EpicsBaseCross(AbstractEpicsBase):

    class BuildTask(AbstractEpicsBase.BuildTask):

        def __init__(self, owner: EpicsBaseCross):
            self._owner = owner
            super().__init__()

        @property
        def owner(self) -> EpicsBaseCross:
            return self._owner

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

    class DeployTask(AbstractEpicsProject.DeployTask):

        def __init__(self, owner: EpicsBaseCross, deploy_path: PurePosixPath):
            self._owner = owner

            super().__init__(
                deploy_path,
                blacklist=[
                    "*.a",
                    "include/*",
                    f"*/{self.owner.host_base.arch}/*",
                ],
            )

        @property
        def owner(self) -> EpicsBaseCross:
            return self._owner

        def dependencies(self) -> List[Task]:
            return [
                *super().dependencies(),
                self.owner.build_task,
            ]

    def __init__(self, target_dir: Path, cc: GccCross, host_base: EpicsBaseHost):

        self._cc = cc

        super().__init__(target_dir / host_base.name, host_base.build_path)

        self.host_base = host_base

        self.deploy_path = PurePosixPath("/opt/epics_base")

        self._build_task = self.BuildTask(self)
        self.deploy_task = self.DeployTask(self, self.deploy_path)

    @property
    def build_task(self) -> BuildTask:
        return self._build_task

    @property
    def cc(self) -> GccCross:
        return self._cc

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
            "deploy": self.deploy_task,
        }
