from __future__ import annotations
from subprocess import Popen
from typing import Any, Dict, List, Optional

import os
import shutil
import re
import logging
import time

from ferrite.utils.files import substitute
from ferrite.components.base import Component, Task, FinalTask, Context
from ferrite.components.toolchains import Toolchain, HostToolchain, CrossToolchain
from ferrite.components.app import App
from ferrite.manage.paths import BASE_DIR, TARGET_DIR
from ferrite.remote.base import Device
from ferrite.components.epics.base import EpicsBuildTask, EpicsDeployTask, epics_arch, epics_host_arch
from ferrite.components.epics.epics_base import EpicsBase, EpicsBaseCross, EpicsBaseHost


class IocBuildTask(EpicsBuildTask):

    def __init__(
        self,
        src_dir: str,
        build_dir: str,
        install_dir: str,
        deps: List[Task],
        epics_base_dir: str,
        app_src_dir: str,
        app_build_dir: str,
        app_fakedev: bool,
        toolchain: Toolchain,
    ):
        super().__init__(
            src_dir,
            build_dir,
            install_dir,
            clean=True,
            deps=deps,
        )
        self.epics_base_dir = epics_base_dir
        self.app_src_dir = app_src_dir
        self.app_build_dir = app_build_dir
        self.app_fakedev = app_fakedev
        self.toolchain = toolchain

    def _configure(self) -> None:
        arch = epics_arch(self.epics_base_dir, self.toolchain)

        substitute([
            ("^\\s*#*(\\s*EPICS_BASE\\s*=).*$", f"\\1 {self.epics_base_dir}"),
        ], os.path.join(self.build_dir, "configure/RELEASE"))

        substitute([
            ("^\\s*#*(\\s*APP_SRC_DIR\\s*=).*$", f"\\1 {self.app_src_dir}"),
            ("^\\s*#*(\\s*APP_BUILD_DIR\\s*=).*$", f"\\1 {self.app_build_dir}"),
        ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

        if not isinstance(self.toolchain, HostToolchain):
            substitute([
                ("^\\s*#*(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {arch}"),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

        substitute([
            ("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {arch}"),
            ("^\\s*#*(\\s*APP_FAKEDEV\\s*=).*$", f"\\1 {'1' if self.app_fakedev else ''}"),
        ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

        substitute([
            ("^\\s*#*(\\s*INSTALL_LOCATION\\s*=).*$", f"\\1 {self.install_dir}"),
        ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))
        os.makedirs(self.install_dir, exist_ok=True)

        #for top_dir in [self.build_dir, self.lib_dir]:
        lib_dir = os.path.join(self.install_dir, "lib", arch)
        lib_name = "libapp{}.so".format("_fakedev" if self.app_fakedev else "")
        os.makedirs(lib_dir, exist_ok=True)
        shutil.copy2(
            os.path.join(self.app_build_dir, lib_name),
            os.path.join(lib_dir, lib_name),
        )

    def _install(self) -> None:
        shutil.copytree(
            os.path.join(self.build_dir, "iocBoot"),
            os.path.join(self.install_dir, "iocBoot"),
            dirs_exist_ok=True,
            ignore=shutil.ignore_patterns("Makefile"),
        )


class IocTestFakeDevTask(Task):

    def __init__(self, owner: Ioc) -> None:
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> None:
        import ferrite.ioc.tests.fakedev as fakedev
        fakedev.run_test(
            self.owner.epics_base.paths["install"],
            self.owner.paths["install"],
            os.path.join(BASE_DIR, "ipp"),
            epics_host_arch(self.owner.epics_base.paths["build"]),
        )

    def dependencies(self) -> List[Task]:
        return [
            self.owner.epics_base.tasks()["build"],
            self.owner.build_task,
        ]


class IocDeployTask(EpicsDeployTask):

    def __init__(
        self,
        install_dir: str,
        deploy_dir: str,
        epics_deploy_path: str,
        deps: List[Task] = [],
    ):
        super().__init__(
            install_dir,
            deploy_dir,
            deps,
        )
        self.epics_deploy_path = epics_deploy_path

    def _post(self, ctx: Context) -> None:
        assert ctx.device is not None
        boot_dir = os.path.join(self.install_dir, "iocBoot")
        for ioc_name in os.listdir(boot_dir):
            ioc_dir = os.path.join(boot_dir, ioc_name)
            if not os.path.isdir(ioc_dir):
                continue
            env_path = os.path.join(ioc_dir, "envPaths")
            if not os.path.isfile(env_path):
                continue
            with open(env_path, "r") as f:
                text = f.read()
            text = re.sub(r'(epicsEnvSet\("TOP",)[^\n]+', f'\\1"{self.deploy_dir}")', text)
            text = re.sub(r'(epicsEnvSet\("EPICS_BASE",)[^\n]+', f'\\1"{self.epics_deploy_path}")', text)
            ctx.device.store_mem(text, os.path.join(self.deploy_dir, "iocBoot", ioc_name, "envPaths"))


class IocRunner:

    def __init__(
        self,
        device: Device,
        deploy_path: str,
        epics_deploy_path: str,
        arch: str,
    ):
        super().__init__()
        self.device = device
        self.deploy_path = deploy_path
        self.epics_deploy_path = epics_deploy_path
        self.arch = arch
        self.proc: Optional[Popen[bytes]] = None

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
            popen=True,
        )
        assert self.proc is not None
        time.sleep(1)
        logging.info("IOC started")

    def __exit__(self, *args: Any) -> None:
        logging.info("terminating IOC ...")
        assert self.proc is not None
        self.proc.terminate()
        logging.info("IOC terminated")


class IocRunTask(FinalTask):

    def __init__(self, owner: Ioc):
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> None:
        assert ctx.device is not None
        assert isinstance(self.owner.epics_base, EpicsBaseCross)
        with IocRunner(
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


class Ioc(Component):

    def __init__(
        self,
        name: str,
        path: str,
        epics_base: EpicsBase,
        app: App,
        toolchain: Toolchain,
    ):
        super().__init__()

        self.name = name
        self.src_path = path
        self.epics_base = epics_base
        self.app = app
        self.toolchain = toolchain

        self.names = {
            "build": f"{self.name}_build_{self.toolchain.name}",
            "install": f"{self.name}_install_{self.toolchain.name}",
        }
        self.paths = {k: os.path.join(TARGET_DIR, v) for k, v in self.names.items()}
        self.deploy_path = "/opt/ioc"

        self.build_task = IocBuildTask(
            self.src_path,
            self.paths["build"],
            self.paths["install"],
            [
                *([self.toolchain.download_task] if isinstance(self.toolchain, CrossToolchain) else []),
                self.epics_base.tasks()["build"],
                self.app.build_main_task if not isinstance(self.toolchain, HostToolchain) else self.app.build_fakedev_task,
            ],
            self.epics_base.paths["build"],
            self.app.src_dir,
            self.app.build_dir,
            isinstance(self.toolchain, HostToolchain),
            self.toolchain,
        )
        if isinstance(self.toolchain, HostToolchain):
            assert isinstance(self.epics_base, EpicsBaseHost)
            self.test_fakedev_task = IocTestFakeDevTask(self)
        elif isinstance(self.toolchain, CrossToolchain):
            assert isinstance(self.epics_base, EpicsBaseCross)
            self.deploy_task = IocDeployTask(
                self.paths["install"],
                self.deploy_path,
                self.epics_base.deploy_path,
                [self.build_task],
            )
            self.run_task = IocRunTask(self)
        else:
            raise RuntimeError("Unknown toolchain type")

    def tasks(self) -> Dict[str, Task]:
        tasks: Dict[str, Task] = {
            "build": self.build_task,
        }
        if isinstance(self.toolchain, HostToolchain):
            tasks.update({
                "test_fakedev": self.test_fakedev_task,
            })
        else:
            tasks.update({
                "deploy": self.deploy_task,
                "run": self.run_task,
            })
        return tasks


class AppIoc(Ioc):

    def __init__(
        self,
        epics_base: EpicsBase,
        app: App,
        toolchain: Toolchain,
    ):
        super().__init__(
            "ioc",
            os.path.join(BASE_DIR, "ioc"),
            epics_base,
            app,
            toolchain,
        )
