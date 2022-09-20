from __future__ import annotations
from typing import List, TypeVar

import shutil
from pathlib import Path, PurePosixPath

from ferrite.utils.path import TargetPath
from ferrite.utils.files import substitute, allow_patterns
from ferrite.components.base import Task, Context
from ferrite.components.git import RepoList, RepoSource
from ferrite.components.compiler import Gcc, GccHost, GccCross
from ferrite.components.epics.base import EpicsInstallTask, EpicsProject, EpicsBuildTask, EpicsDeployTask, C

import logging

logger = logging.getLogger(__name__)


class AbstractEpicsBase(EpicsProject[C]):

    def __init__(self, src_dir: Path | TargetPath, target_dir: TargetPath, cc: C) -> None:
        super().__init__(src_dir, target_dir, cc.name)
        self._cc = cc

    @property
    def cc(self) -> C:
        return self._cc

    @property
    def build_task(self) -> _BuildTask[AbstractEpicsBase[C]]:
        raise NotImplementedError()

    @property
    def install_task(self) -> _InstallTask[AbstractEpicsBase[C]]:
        raise NotImplementedError()


class EpicsBaseHost(AbstractEpicsBase[GccHost]):

    def __init__(self, cc: GccHost):

        self.version = "7.0.6.1"
        name = f"epics_base_{self.version}"

        super().__init__(TargetPath(name) / "src", TargetPath(name), cc)

        self.name = name

        assert isinstance(self.src_dir, TargetPath)
        self.repo = RepoList(
            self.src_dir,
            [
                RepoSource("https://gitlab.inp.nsk.su/epics/epics-base.git", f"binp-R{self.version}"),
                RepoSource("https://github.com/epics-base/epics-base.git", f"R{self.version}"),
            ],
            cached=True,
        )

        self._build_task = _HostBuildTask(self)
        self._install_task = _HostInstallTask(self)

    @property
    def build_task(self) -> _HostBuildTask:
        return self._build_task

    @property
    def install_task(self) -> _HostInstallTask:
        return self._install_task


class EpicsBaseCross(AbstractEpicsBase[GccCross]):

    def __init__(self, cc: GccCross, host_base: EpicsBaseHost):

        super().__init__(host_base.build_dir, TargetPath(host_base.name), cc)

        self.host_base = host_base

        self.deploy_path = PurePosixPath("/opt/epics_base")

        self._build_task = _CrossBuildTask(self)
        self._install_task = _CrossInstallTask(self)
        self.deploy_task = _CrossDeployTask(self, self.deploy_path)

    @property
    def build_task(self) -> _CrossBuildTask:
        return self._build_task

    @property
    def install_task(self) -> _CrossInstallTask:
        return self._install_task


O = TypeVar("O", bound=AbstractEpicsBase[Gcc], covariant=True)


class _BuildTask(EpicsBuildTask[O]):

    def __init__(self, owner: O) -> None:
        super().__init__(owner, clean=False, cached=True)

    def _configure_common(self, ctx: Context) -> None:
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
            ctx.target_path / self.owner.build_dir / "configure/CONFIG_COMMON",
        )

    def _configure_toolchain(self, ctx: Context) -> None:
        raise NotImplementedError()

    def _configure_install(self, ctx: Context) -> None:
        substitute([
            ("^\\s*#*(\\s*INSTALL_LOCATION\\s*=).*$", f"\\1 {ctx.target_path / self.owner.install_dir}"),
        ], ctx.target_path / self.owner.build_dir / "configure/CONFIG_SITE")

    def _configure(self, ctx: Context) -> None:
        self._configure_common(ctx)
        self._configure_toolchain(ctx)
        #self._configure_install(ctx) # Install is broken


class _InstallTask(EpicsInstallTask[O]):

    # Workaround for broken EPICS install
    def _install(self, ctx: Context) -> None:
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
            shutil.rmtree(
                ctx.target_path / self.owner.install_dir / path,
                ignore_errors=True,
            )
            shutil.copytree(
                ctx.target_path / self.owner.build_dir / path,
                ctx.target_path / self.owner.install_dir / path,
                symlinks=True,
                ignore=shutil.ignore_patterns("O.*"),
            )


class _HostBuildTask(_BuildTask[EpicsBaseHost]):

    def _configure_toolchain(self, ctx: Context) -> None:
        substitute(
            [("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", "\\1")],
            ctx.target_path / self.owner.build_dir / "configure/CONFIG_SITE",
        )

    def dependencies(self) -> List[Task]:
        return [
            *super().dependencies(),
            self.owner.repo.clone_task,
        ]


class _HostInstallTask(_InstallTask[EpicsBaseHost]):
    pass


class _CrossBuildTask(_BuildTask[EpicsBaseCross]):

    def _configure_toolchain(self, ctx: Context) -> None:
        cc = self.owner.cc

        host_arch = self.owner.host_base.arch
        cross_arch = self.owner.arch
        assert cross_arch != host_arch

        if cross_arch == "linux-arm" and host_arch.endswith("-x86_64"):
            host_arch = host_arch[:-3] # Trim '_64'

        substitute(
            [("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {cross_arch}")],
            ctx.target_path / self.owner.build_dir / "configure/CONFIG_SITE",
        )
        substitute(
            [
                ("^(\\s*GNU_TARGET\\s*=).*$", f"\\1 {str(cc.target)}"),
                ("^(\\s*GNU_DIR\\s*=).*$", f"\\1 {ctx.target_path / cc.path}"),
            ],
            ctx.target_path / self.owner.build_dir / f"configure/os/CONFIG_SITE.{host_arch}.{cross_arch}",
        )

    def dependencies(self) -> List[Task]:
        return [
            *super().dependencies(),
            self.owner.host_base.build_task,
        ]


class _CrossInstallTask(_InstallTask[EpicsBaseCross]):

    def _install(self, ctx: Context) -> None:
        super()._install(ctx)

        host_arch = self.owner.host_base.arch
        cross_arch = self.owner.arch
        assert cross_arch != host_arch

        shutil.copytree(
            ctx.target_path / self.owner.build_dir / "bin" / host_arch,
            ctx.target_path / self.owner.install_dir / "bin" / cross_arch,
            dirs_exist_ok=True,
            symlinks=True,
            ignore=allow_patterns("*.pl", "*.py"),
        )


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
