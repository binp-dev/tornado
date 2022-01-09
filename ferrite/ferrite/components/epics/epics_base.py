from __future__ import annotations
import os
import shutil
import logging
from ferrite.utils.files import substitute, allow_patterns
from ferrite.components.base import Component, Task, Context
from ferrite.components.git import RepoList, RepoSource
from ferrite.components.toolchains import Toolchain, HostToolchain, CrossToolchain
from ferrite.manage.paths import TARGET_DIR
from .base import EpicsBuildTask, EpicsDeployTask, epics_arch, epics_host_arch, epics_arch_by_target


class EpicsBaseBuildTask(EpicsBuildTask):

    def __init__(
        self,
        src_dir: str,
        build_dir: str,
        install_dir: str,
        deps: list[Task],
        toolchain: Toolchain,
    ):
        super().__init__(
            src_dir,
            build_dir,
            install_dir,
            deps=deps,
        )
        self.toolchain = toolchain

    def _configure_common(self):
        defs = [
            #("USR_CFLAGS", ""),
            #("USR_CPPFLAGS", ""),
            ("USR_CXXFLAGS", "-std=c++17"),
            ("BIN_PERMISSIONS", "755"),
            ("LIB_PERMISSIONS", "644"),
            ("SHRLIB_PERMISSIONS", "755"),
            ("INSTALL_PERMISSIONS", "644"),
        ]
        rules = [(f"^(\\s*{k}\\s*=).*$", f"\\1 {v}") for k, v in defs]
        logging.warning(rules)
        substitute(rules, os.path.join(self.build_dir, "configure/CONFIG_COMMON"))

    def _configure_toolchain(self):
        if isinstance(self.toolchain, HostToolchain):
            substitute([
                ("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", "\\1"),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

        elif isinstance(self.toolchain, CrossToolchain):
            host_arch = epics_host_arch(self.src_dir)
            cross_arch = epics_arch_by_target(self.toolchain.target)
            if cross_arch == "linux-arm" and host_arch.endswith("-x86_64"):
                host_arch = host_arch[:-3]  # Trim '_64'

            substitute([
                ("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {cross_arch}"),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

            substitute([
                ("^(\\s*GNU_TARGET\\s*=).*$", f"\\1 {str(self.toolchain.target)}"),
                ("^(\\s*GNU_DIR\\s*=).*$", f"\\1 {self.toolchain.path}"),
            ], os.path.join(self.build_dir, f"configure/os/CONFIG_SITE.{host_arch}.{cross_arch}"))

        else:
            raise RuntimeError(f"Unsupported toolchain type: {type(self.toolchain).__name__}")

    def _configure_install(self):
        substitute([
            ("^\\s*#*(\\s*INSTALL_LOCATION\\s*=).*$", f"\\1 {self.install_dir}"),
        ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

    def _configure(self):
        self._configure_common()
        self._configure_toolchain()
        #self._configure_install()

    def _install(self):
        host_arch = epics_host_arch(self.build_dir)
        arch = epics_arch(self.build_dir, self.toolchain)
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
                os.path.join(self.build_dir, path),
                os.path.join(self.install_dir, path),
                dirs_exist_ok=True,
                symlinks=True,
                ignore=shutil.ignore_patterns("O.*"),
            )
        if arch != host_arch:
            shutil.copytree(
                os.path.join(self.build_dir, "bin", host_arch),
                os.path.join(self.install_dir, "bin", arch),
                dirs_exist_ok=True,
                symlinks=True,
                ignore=allow_patterns("*.pl", "*.py"),
            )

    def run(self, ctx: Context) -> bool:
        done_path = os.path.join(self.build_dir, "build.done")
        if os.path.exists(done_path):
            with open(done_path, "r") as f:
                if f.read() == self.build_dir:
                    logging.info(f"'{self.build_dir}' is already built")
                    return False

        super().run(ctx)
        with open(done_path, "w") as f:
            f.write(self.build_dir)
        return True


class EpicsBase(Component):

    def __init__(self):
        super().__init__()


class EpicsBaseHost(EpicsBase):

    def __init__(self, toolchain: HostToolchain):
        super().__init__()

        self.toolchain = toolchain

        self.version = "7.0.4.1"
        self.prefix = f"epics_base_{self.version}"
        self.src_name = f"{self.prefix}_src"
        self.src_path = os.path.join(TARGET_DIR, self.src_name)
        self.repo = RepoList(
            self.src_name,
            [
                RepoSource("https://gitlab.inp.nsk.su/epics/epics-base.git", f"binp-R{self.version}"),
                RepoSource("https://github.com/epics-base/epics-base.git", f"R{self.version}"),
            ],
        )

        self.names = {
            "build": f"{self.prefix}_build_{self.toolchain.name}",
            "install": f"{self.prefix}_install_{self.toolchain.name}",
        }
        self.paths = {k: os.path.join(TARGET_DIR, v) for k, v in self.names.items()}

        self.build_task = EpicsBaseBuildTask(
            self.src_path,
            self.paths["build"],
            self.paths["install"],
            [self.repo.clone_task],
            self.toolchain,
        )

    def arch(self) -> str:
        return epics_host_arch(self.src_path)

    def tasks(self) -> dict[str, Task]:
        return {
            "clone": self.repo.clone_task,
            "build": self.build_task,
        }


class EpicsBaseCross(EpicsBase):

    def __init__(self, toolchain: CrossToolchain, host_base: EpicsBaseHost):
        super().__init__()

        self.toolchain = toolchain
        self.host_base = host_base

        self.prefix = self.host_base.prefix

        self.names = {
            "build": f"{self.prefix}_build_{self.toolchain.name}",
            "install": f"{self.prefix}_install_{self.toolchain.name}",
        }
        self.paths = {k: os.path.join(TARGET_DIR, v) for k, v in self.names.items()}
        self.deploy_path = "/opt/epics_base"

        self.build_task = EpicsBaseBuildTask(
            self.host_base.paths["build"],
            self.paths["build"],
            self.paths["install"],
            [
                self.toolchain.download_task,
                self.host_base.build_task,
            ],
            self.toolchain,
        )
        self.deploy_task = EpicsDeployTask(
            self.paths["install"],
            self.deploy_path,
            [self.build_task],
        )

    def arch(self) -> str:
        return epics_arch_by_target(self.toolchain.target)

    def tasks(self) -> dict[str, Task]:
        return {
            "build": self.build_task,
            "deploy": self.deploy_task,
        }
