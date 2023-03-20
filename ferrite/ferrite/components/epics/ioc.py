from __future__ import annotations
from typing import Generic, List, TypeVar

import shutil
import re
import time
from pathlib import Path, PurePosixPath

from ferrite.utils.path import TargetPath
from ferrite.utils.files import substitute
from ferrite.components.base import Artifact, OwnedTask, Task, Context
from ferrite.components.compiler import Gcc, GccHost, GccCross
from ferrite.components.epics.base import EpicsInstallTask, EpicsProject, EpicsBuildTask, EpicsDeployTask
from ferrite.components.epics.epics_base import AbstractEpicsBase, EpicsBaseCross, EpicsBaseHost
from ferrite.utils.epics.ioc_remote import IocRemoteRunner

# EpicsBase
B = TypeVar("B", bound=AbstractEpicsBase[Gcc], covariant=True)


class AbstractIoc(EpicsProject[Gcc], Generic[B]):

    def BuildTask(self) -> AbstractBuildTask[AbstractIoc[B]]:
        raise NotImplementedError()

    def __init__(self, ioc_dirs: List[Path], target_dir: TargetPath, epics_base: B):
        src_dir: Path | TargetPath
        assert len(ioc_dirs) > 0
        if len(ioc_dirs) == 1:
            src_dir = ioc_dirs[0]
        else:
            # Create separate source dir
            src_dir = target_dir / f"src"

        super().__init__(src_dir, target_dir, epics_base.cc.name)

        self.epics_base = epics_base
        self.ioc_dirs = ioc_dirs
        self._build_task = self.BuildTask()
        self._install_task = AbstractInstallTask(self)

    @property
    def build_task(self) -> AbstractBuildTask[AbstractIoc[B]]:
        return self._build_task

    @property
    def install_task(self) -> AbstractInstallTask[AbstractIoc[B]]:
        return self._install_task

    @property
    def arch(self) -> str:
        return self.epics_base.arch


class IocHost(AbstractIoc[EpicsBaseHost]):

    def BuildTask(self) -> HostBuildTask[IocHost]:
        return HostBuildTask(self)

    def __init__(self, ioc_dirs: List[Path], target_dir: TargetPath, epics_base: EpicsBaseHost):
        super().__init__(ioc_dirs, target_dir, epics_base)

    @property
    def cc(self) -> GccHost:
        return self.epics_base.cc


class IocCross(AbstractIoc[EpicsBaseCross]):

    def BuildTask(self) -> CrossBuildTask[IocCross]:
        return CrossBuildTask(self)

    def DeployTask(self) -> _CrossDeployTask[IocCross]:
        return _CrossDeployTask(
            self,
            self.deploy_path,
            self.epics_base.deploy_path,
        )

    def __init__(self, ioc_dirs: List[Path], target_dir: TargetPath, epics_base: EpicsBaseCross):
        super().__init__(ioc_dirs, target_dir, epics_base)

        self.deploy_path = PurePosixPath("/opt/ioc")

        self.deploy_task = self.DeployTask()
        self.run_task = _CrossRunTask(self)

    @property
    def cc(self) -> GccCross:
        return self.epics_base.cc


O = TypeVar("O", bound=AbstractIoc[AbstractEpicsBase[Gcc]], covariant=True)


class AbstractBuildTask(EpicsBuildTask[O]):

    def __init__(self, owner: O):
        super().__init__(owner, clean=True)

    def _prepare_source(self, ctx: Context) -> None:
        if len(self.owner.ioc_dirs) == 1:
            # There is no patches
            return

        for ioc_dirs in self.owner.ioc_dirs:
            shutil.copytree(ioc_dirs, ctx.target_path / self.owner.src_dir, dirs_exist_ok=True)

    def _dep_paths(self, ctx: Context) -> List[Path]:
        return self.owner.ioc_dirs

    def _configure(self, ctx: Context) -> None:
        build_path = ctx.target_path / self.owner.build_dir
        install_path = ctx.target_path / self.owner.install_dir
        substitute(
            [("^\\s*#*(\\s*EPICS_BASE\\s*=).*$", f"\\1 {ctx.target_path / self.owner.epics_base.build_dir}")],
            build_path / "configure/RELEASE",
        )
        substitute(
            [("^\\s*#*(\\s*INSTALL_LOCATION\\s*=).*$", f"\\1 {install_path}")],
            build_path / "configure/CONFIG_SITE",
        )
        install_path.mkdir(exist_ok=True)

    def dependencies(self) -> List[Task]:
        return [
            *super().dependencies(),
            self.owner.epics_base.build_task,
        ]

    def artifacts(self) -> List[Artifact]:
        assert isinstance(self.owner.src_dir, TargetPath)
        return [
            *super().artifacts(),
            *([Artifact(self.owner.src_dir)] if len(self.owner.ioc_dirs) > 1 else []),
            Artifact(self.owner.install_dir), # Because IOC install is performed during build
        ]


class AbstractInstallTask(EpicsInstallTask[O]):

    def _install(self, ctx: Context) -> None:
        shutil.rmtree(
            ctx.target_path / self.owner.install_dir / "iocBoot",
            ignore_errors=True,
        )
        shutil.copytree(
            ctx.target_path / self.owner.build_dir / "iocBoot",
            ctx.target_path / self.owner.install_dir / "iocBoot",
            dirs_exist_ok=True,
            ignore=shutil.ignore_patterns("Makefile"),
        )


Ho = TypeVar("Ho", bound=IocHost, covariant=True)
Co = TypeVar("Co", bound=IocCross, covariant=True)


class HostBuildTask(AbstractBuildTask[Ho]):
    pass


class CrossBuildTask(AbstractBuildTask[Co]):

    def _configure(self, ctx: Context) -> None:
        super()._configure(ctx)
        substitute(
            [("^\\s*#*(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {self.owner.arch}")],
            ctx.target_path / self.owner.build_dir / "configure/CONFIG_SITE",
        )


class _CrossDeployTask(EpicsDeployTask[Co]):

    def __init__(
        self,
        owner: Co,
        deploy_dir: PurePosixPath,
        epics_deploy_path: PurePosixPath,
    ):
        super().__init__(owner, deploy_dir)
        self.epics_deploy_path = epics_deploy_path

    def _post(self, ctx: Context) -> None:
        assert ctx.device is not None
        boot_dir = ctx.target_path / self.owner.install_dir / "iocBoot"
        for ioc_name in [path.name for path in boot_dir.iterdir()]:
            ioc_dirs = boot_dir / ioc_name
            if not ioc_dirs.is_dir():
                continue
            env_path = ioc_dirs / "envPaths"
            if not env_path.is_file():
                continue
            with open(env_path, "r") as f:
                text = f.read()
            text = re.sub(r'(epicsEnvSet\("TOP",)[^\n]+', f'\\1"{self.deploy_path}")', text)
            text = re.sub(r'(epicsEnvSet\("EPICS_BASE",)[^\n]+', f'\\1"{self.epics_deploy_path}")', text)
            ctx.device.store_mem(text, self.deploy_path / "iocBoot" / ioc_name / "envPaths")


class _CrossRunTask(OwnedTask[Co]):

    def run(self, ctx: Context) -> None:
        assert ctx.device is not None
        assert isinstance(self.owner.epics_base, EpicsBaseCross)
        with IocRemoteRunner(
            ctx.device,
            self.owner.deploy_path,
            self.owner.epics_base.deploy_path,
            self.owner.epics_base.arch,
        ):
            try:
                while True:
                    time.sleep(1)
            except KeyboardInterrupt:
                pass

    def dependencies(self) -> List[Task]:
        return [
            self.owner.epics_base.deploy_task,
            self.owner.deploy_task,
        ]
