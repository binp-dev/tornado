from __future__ import annotations
from typing import Dict, List

import shutil
import re
import time
from pathlib import Path, PurePosixPath

from ferrite.utils.files import substitute
from ferrite.components.base import Task, FinalTask, Context
from ferrite.components.toolchain import HostToolchain, CrossToolchain
from ferrite.components.epics.base import AbstractEpicsProject
from ferrite.components.epics.epics_base import AbstractEpicsBase, EpicsBaseCross, EpicsBaseHost
from ferrite.utils.epics.ioc_remote import IocRemoteRunner


class AbstractIoc(AbstractEpicsProject):

    class BuildTask(AbstractEpicsProject.BuildTask):

        def __init__(
            self,
            owner: AbstractIoc,
            deps: List[Task],
        ):
            self._owner = owner
            super().__init__(deps=deps, clean=True)
            self.epics_base_dir = self.owner.epics_base.build_path

        @property
        def owner(self) -> AbstractIoc:
            return self._owner

        def _prepare_source(self) -> None:
            if len(self.owner.ioc_dirs) == 1:
                # There is no patches
                return

            for ioc_dirs in self.owner.ioc_dirs:
                shutil.copytree(ioc_dirs, self.src_dir, dirs_exist_ok=True)

        def _dep_paths(self) -> List[Path]:
            return self.owner.ioc_dirs

        def _configure(self) -> None:
            substitute(
                [("^\\s*#*(\\s*EPICS_BASE\\s*=).*$", f"\\1 {self.epics_base_dir}")],
                self.build_dir / "configure/RELEASE",
            )
            substitute(
                [("^\\s*#*(\\s*INSTALL_LOCATION\\s*=).*$", f"\\1 {self.install_dir}")],
                self.build_dir / "configure/CONFIG_SITE",
            )
            self.install_dir.mkdir(exist_ok=True)

        def _install(self) -> None:
            shutil.copytree(
                self.build_dir / "iocBoot",
                self.install_dir / "iocBoot",
                dirs_exist_ok=True,
                ignore=shutil.ignore_patterns("Makefile"),
            )

    def _build_deps(self) -> List[Task]:
        return [self.epics_base.build_task]

    def _make_build_task(self) -> AbstractIoc.BuildTask:
        return self.BuildTask(self, deps=self._build_deps())

    def __init__(
        self,
        name: str,
        ioc_dirs: List[Path],
        target_dir: Path,
        epics_base: AbstractEpicsBase,
    ):
        assert len(ioc_dirs) > 0
        if len(ioc_dirs) == 1:
            src_dir = ioc_dirs[0]
        else:
            # Create separate source dir
            src_dir = target_dir / f"{name}_src"

        self._epics_base = epics_base
        super().__init__(target_dir, src_dir, name)
        self.ioc_dirs = ioc_dirs
        self.build_task = self._make_build_task()

    @property
    def epics_base(self) -> AbstractEpicsBase:
        return self._epics_base

    @property
    def arch(self) -> str:
        return self.epics_base.arch

    def tasks(self) -> Dict[str, Task]:
        return {"build": self.build_task}


class IocHost(AbstractIoc):

    def __init__(
        self,
        name: str,
        ioc_dirs: List[Path],
        target_dir: Path,
        epics_base: EpicsBaseHost,
    ):
        self._epics_base_host = epics_base
        super().__init__(name, ioc_dirs, target_dir, epics_base)
        self.build_task = self._make_build_task()

    @property
    def epics_base(self) -> EpicsBaseHost:
        return self._epics_base_host

    @property
    def toolchain(self) -> HostToolchain:
        return self.epics_base.toolchain


class IocCross(AbstractIoc):

    class BuildTask(AbstractIoc.BuildTask):

        def _configure(self) -> None:
            super()._configure()
            substitute(
                [("^\\s*#*(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {self.owner.arch}")],
                self.build_dir / "configure/CONFIG_SITE",
            )

    class DeployTask(AbstractEpicsProject.DeployTask):

        def __init__(
            self,
            owner: IocCross,
            deploy_dir: PurePosixPath,
            epics_deploy_path: PurePosixPath,
            deps: List[Task],
        ):
            self._owner = owner
            super().__init__(deploy_dir, deps)
            self.epics_deploy_path = epics_deploy_path

        @property
        def owner(self) -> IocCross:
            return self._owner

        def _post(self, ctx: Context) -> None:
            assert ctx.device is not None
            boot_dir = self.owner.install_path / "iocBoot"
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

    class RunTask(FinalTask):

        def __init__(self, owner: IocCross):
            super().__init__()
            self.owner = owner

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
            assert isinstance(self.owner.epics_base, EpicsBaseCross)
            return [
                self.owner.epics_base.deploy_task,
                self.owner.deploy_task,
            ]

    def _build_deps(self) -> List[Task]:
        return [
            self.toolchain.download_task,
            *super()._build_deps(),
        ]

    def __init__(
        self,
        name: str,
        ioc_dirs: List[Path],
        target_dir: Path,
        epics_base: EpicsBaseCross,
    ):
        self._epics_base_cross = epics_base
        super().__init__(name, ioc_dirs, target_dir, epics_base)

        self.deploy_path = PurePosixPath("/opt/ioc")

        self.deploy_task = self.DeployTask(
            self,
            self.deploy_path,
            self.epics_base.deploy_path,
            [self.build_task],
        )
        self.run_task = self.RunTask(self)

    @property
    def epics_base(self) -> EpicsBaseCross:
        return self._epics_base_cross

    @property
    def toolchain(self) -> CrossToolchain:
        return self.epics_base.toolchain

    def tasks(self) -> Dict[str, Task]:
        tasks = super().tasks()
        tasks.update({
            "deploy": self.deploy_task,
            "run": self.run_task,
        })
        return tasks
