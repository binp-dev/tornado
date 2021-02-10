from __future__ import annotations
import os
import shutil
import logging
from utils.files import substitute, allow_patterns
from manage.components.base import Component, Task
from manage.components.git import Repo
from manage.components.toolchains import Toolchain
from manage.paths import TARGET_DIR
from .base import EpicsBuildTask, EpicsDeployTask, epics_arch, epics_host_arch, epics_arch_by_target

class EpicsBaseBuildTask(EpicsBuildTask):
    def __init__(
        self,
        src_dir: src,
        build_dir: src,
        install_dir: src,
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
        if self.toolchain is None:
            substitute([
                ("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", "\\1"),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

        else:
            host_arch = epics_host_arch(self.src_dir)
            if host_arch.endswith("-x86_64"):
                # Trim '_64'
                host_arch = host_arch[:-3]
            cross_arch = epics_arch_by_target(self.toolchain.target)
            
            substitute([
                ("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {cross_arch}"),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

            substitute([
                ("^(\\s*GNU_TARGET\\s*=).*$", f"\\1 {self.toolchain.target}"),
                ("^(\\s*GNU_DIR\\s*=).*$", f"\\1 {self.toolchain.path}"),
            ], os.path.join(self.build_dir, f"configure/os/CONFIG_SITE.{host_arch}.{cross_arch}"))

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
        arch = epics_arch(self.build_dir, self.toolchain and self.toolchain.target)
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
    def __init__(self, cross_toolchain: Toolchain=None):
        super().__init__()

        self.src_name = "epics_base_src"
        self.src_path = os.path.join(TARGET_DIR, self.src_name)
        self.repo = Repo(
            "https://github.com/epics-base/epics-base.git",
            "epics_base_src",
            "R7.0.4.1",
        )
        self.cross_toolchain = cross_toolchain

        self.names = {
            "host_build":    "epics_base_host_build",
            "cross_build":   "epics_base_cross_build",
            "host_install":  "epics_base_host_install",
            "cross_install": "epics_base_cross_install",
        }
        self.paths = {k: os.path.join(TARGET_DIR, v) for k, v in self.names.items()}
        self.deploy_path = "/opt/epics_base"

        self.host_build_task = EpicsBaseBuildTask(
            self.src_path,
            self.paths["host_build"],
            self.paths["host_install"],
            [self.repo.clone_task],
            None,
        )
        self.cross_build_task = EpicsBaseBuildTask(
            self.paths["host_build"],
            self.paths["cross_build"],
            self.paths["cross_install"],
            [self.host_build_task],
            self.cross_toolchain,
        )
        self.deploy_task = EpicsDeployTask(
            self.paths["cross_install"],
            self.deploy_path,
            [self.cross_build_task],
        )

    def host_arch(self) -> str:
        return epics_host_arch(self.src_path)
    
    def cross_arch(self) -> str:
        tc = self.cross_toolchain
        return tc and epics_arch_by_target(tc.target)

    def tasks(self) -> dict[str, Task]:
        return {
            "clone": self.repo.clone_task,
            "build_host": self.host_build_task,
            "build_cross": self.cross_build_task,
            "deploy": self.deploy_task,
        }
