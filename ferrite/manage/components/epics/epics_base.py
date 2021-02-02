from __future__ import annotations
import os
import logging
from utils.files import substitute
from manage.components.base import Component, Task
from manage.components.git import Repo
from manage.components.toolchains import Toolchain
from manage.paths import TARGET_DIR
from .base import EpicsBuildTask, epics_host_arch, epics_arch_by_target

class EpicsBaseBuildTask(EpicsBuildTask):
    def __init__(
        self,
        src_dir: src,
        build_dir: src,
        deps: list[Task],
        toolchain: Toolchain,
    ):
        super().__init__(
            src_dir,
            build_dir,
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

    def _configure(self):
        self._configure_common()
        self._configure_toolchain()

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
        }
        self.paths = {k: os.path.join(TARGET_DIR, v) for k, v in self.names.items()}

        self.host_build_task = EpicsBaseBuildTask(
            self.src_path,
            self.paths["host_build"],
            [self.repo.clone_task],
            None,
        )
        self.cross_build_task = EpicsBaseBuildTask(
            self.paths["host_build"],
            self.paths["cross_build"],
            [self.host_build_task],
            self.cross_toolchain,
        )

    def tasks(self) -> dict[str, Task]:
        return {
            "clone": self.repo.clone_task,
            "build_host": self.host_build_task,
            "build_cross": self.cross_build_task,
        }
