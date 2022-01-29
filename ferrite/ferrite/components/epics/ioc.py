from __future__ import annotations
from typing import Any, Dict, List, Optional

import shutil
import re
import time
from pathlib import Path, PurePosixPath
import logging

from ferrite.utils.files import substitute
from ferrite.components.base import Task, FinalTask, Context
from ferrite.components.toolchain import HostToolchain, CrossToolchain, Toolchain
from ferrite.remote.base import Device, Connection
from ferrite.components.epics.base import epics_arch, Epics
from ferrite.components.epics.epics_base import EpicsBase, EpicsBaseCross, EpicsBaseHost


class IocRemoteRunner:

    def __init__(
        self,
        device: Device,
        deploy_path: PurePosixPath,
        epics_deploy_path: PurePosixPath,
        arch: str,
    ):
        super().__init__()
        self.device = device
        self.deploy_path = deploy_path
        self.epics_deploy_path = epics_deploy_path
        self.arch = arch
        self.proc: Optional[Connection] = None

    def __enter__(self) -> None:
        self.proc = self.device.run(
            [
                "bash",
                "-c",
                "export {}; export {}; cd {} && {} {}".format(
                    f"TOP={self.deploy_path}",
                    f"LD_LIBRARY_PATH={self.epics_deploy_path}/lib/{self.arch}:{self.deploy_path}/lib/{self.arch}",
                    f"{self.deploy_path}/iocBoot/iocPSC",
                    f"{self.deploy_path}/bin/{self.arch}/PSC",
                    "st.cmd",
                ),
            ],
            wait=False,
        )
        assert self.proc is not None
        time.sleep(1)
        logging.info("IOC started")

    def __exit__(self, *args: Any) -> None:
        logging.info("terminating IOC ...")
        assert self.proc is not None
        self.proc.close()
        logging.info("IOC terminated")


class Ioc(Epics):

    class BuildTask(Epics.BuildTask):

        def __init__(
            self,
            owner: Ioc,
            deps: List[Task],
        ):
            self._owner = owner
            super().__init__(deps=deps, clean=True)
            self.epics_base_dir = self.owner.epics_base.build_path

        @property
        def owner(self) -> Ioc:
            return self._owner

        def _configure(self) -> None:
            toolchain = self.owner.toolchain
            arch = epics_arch(self.epics_base_dir, toolchain)
            substitute(
                [("^\\s*#*(\\s*EPICS_BASE\\s*=).*$", f"\\1 {self.epics_base_dir}")],
                self.build_dir / "configure/RELEASE",
            )
            if not isinstance(toolchain, HostToolchain):
                substitute(
                    [("^\\s*#*(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {arch}")],
                    self.build_dir / "configure/CONFIG_SITE",
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

    def _make_build_task(self) -> Ioc.BuildTask:
        return self.BuildTask(self, deps=self._build_deps())

    def __init__(
        self,
        name: str,
        ioc_dir: Path,
        target_dir: Path,
        epics_base: EpicsBase,
    ):
        self._epics_base = epics_base
        super().__init__(target_dir, ioc_dir, name)
        self.build_task = self._make_build_task()

    @property
    def epics_base(self) -> EpicsBase:
        return self._epics_base

    def tasks(self) -> Dict[str, Task]:
        return {"build": self.build_task}


class IocHost(Ioc):

    def __init__(
        self,
        name: str,
        ioc_dir: Path,
        target_dir: Path,
        epics_base: EpicsBaseHost,
    ):
        self._epics_base_host = epics_base
        super().__init__(name, ioc_dir, target_dir, epics_base)
        self.build_task = self._make_build_task()

    @property
    def epics_base(self) -> EpicsBaseHost:
        return self._epics_base_host

    @property
    def toolchain(self) -> HostToolchain:
        return self.epics_base.toolchain


class IocCross(Ioc):

    class DeployTask(Epics.DeployTask):

        def __init__(
            self,
            install_dir: Path,
            deploy_dir: PurePosixPath,
            epics_deploy_path: PurePosixPath,
            deps: List[Task],
        ):
            super().__init__(install_dir, deploy_dir, deps)
            self.epics_deploy_path = epics_deploy_path

        def _post(self, ctx: Context) -> None:
            assert ctx.device is not None
            boot_dir = self.install_dir / "iocBoot"
            for ioc_name in [path.name for path in boot_dir.iterdir()]:
                ioc_dir = boot_dir / ioc_name
                if not ioc_dir.is_dir():
                    continue
                env_path = ioc_dir / "envPaths"
                if not env_path.is_file():
                    continue
                with open(env_path, "r") as f:
                    text = f.read()
                text = re.sub(r'(epicsEnvSet\("TOP",)[^\n]+', f'\\1"{self.deploy_dir}")', text)
                text = re.sub(r'(epicsEnvSet\("EPICS_BASE",)[^\n]+', f'\\1"{self.epics_deploy_path}")', text)
                ctx.device.store_mem(text, self.deploy_dir / "iocBoot" / ioc_name / "envPaths")

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
                self.owner.epics_base.arch(),
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

    def __init__(
        self,
        name: str,
        ioc_dir: Path,
        target_dir: Path,
        epics_base: EpicsBaseCross,
    ):
        self._epics_base_cross = epics_base
        super().__init__(name, ioc_dir, target_dir, epics_base)

        self.deploy_path = PurePosixPath("/opt/ioc")

        self.deploy_task = self.DeployTask(
            self.install_path,
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
