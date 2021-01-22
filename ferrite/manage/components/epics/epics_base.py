from __future__ import annotations
import os
import logging
from manage.components.base import Component, Task
from manage.components.git import Repo
from manage.components.toolchains import Toolchain
from manage.utils.files import substitute
from manage.paths import TARGET_DIR
from .base import EpicsBuildTask, epics_host_arch, epics_arch_by_target

class EpicsBaseBuildTask(EpicsBuildTask):
    def __init__(self, base_args, toolchain: Toolchain, **kwargs):
        super().__init__(*base_args, **kwargs)
        self.toolchain = toolchain

    def _configure_common(self):
        usr_vars = [
            #("USR_CFLAGS", ""),
            ("USR_CXXFLAGS", "-std=c++17"),
            #("USR_CPPFLAGS", ""),
        ]
        rules = [(r'^([ \t]*{}[ \t]*=[ \t]*)[^\n]*$'.format(v), r'\1{}'.format(f)) for v, f in usr_vars]
        substitute(rules, os.path.join(self.build_dir, "configure/CONFIG_COMMON"))

    def _configure_toolchain(self):
        if self.toolchain is None:
            substitute([
                (r'^([ \t]*CROSS_COMPILER_TARGET_ARCHS[ \t]*=[ \t]*)[^\n]*$', r'\1'),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))
            
        else:
            host_arch = epics_host_arch(self.src_dir)
            if host_arch.endswith("-x86_64"):
                # Trim '_64'
                host_arch = host_arch[:-3]
            cross_arch = epics_arch_by_target(self.toolchain.target)
            
            substitute([
                (r'^([ \t]*CROSS_COMPILER_TARGET_ARCHS[ \t]*=[ \t]*)[^\n]*$', r'\1{}'.format(cross_arch)),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

            substitute([
                (r'^([ \t]*GNU_TARGET[ \t]*=[ \t]*)[^\n]*$', r'\1{}'.format(self.toolchain.target)),
                (r'^([ \t]*GNU_DIR[ \t]*=[ \t]*)[^\n]*$', r'\1{}'.format(self.toolchain.path)),
            ], os.path.join(self.build_dir, f"configure/os/CONFIG_SITE.{host_arch}.{cross_arch}"))

    def _configure_install(self):
        if self.install_dir is None:
            substitute([
                (r'^[ \t]*#*([ \t]*INSTALL_LOCATION[ \t]*=[ \t]*)[^\n]*$', r'#\1'),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

        else:
            substitute([
                (r'^[ \t]*#*([ \t]*INSTALL_LOCATION[ \t]*=[ \t]*)[^\n]*$', r'\1{}'.format(self.install_dir)),
            ], os.path.join(self.build_dir, "configure/CONFIG_SITE"))

    def _configure(self):
        self._configure_common()
        self._configure_toolchain()
        self._configure_install()

    def _variables(self) -> dict[str, str]:
        return {}

class EpicsBase(Component):
    def __init__(self, cross_toolchain: Toolchain=None):
        super().__init__()

        self.src_name = "epics_base_src"
        self.src_path = os.path.join(TARGET_DIR, self.src_name)
        self.repo = Repo(
            "https://github.com/epics-base/epics-base.git",
            "epics_base_src",
            "R7.0.3.1",
        )
        self.cross_toolchain = cross_toolchain

        self.names = {
            "host_build":    "epics_base_host_build",
            "cross_build":   "epics_base_cross_build",
            "host_install":  "epics_base_host_install",
            "cross_install": "epics_base_cross_install",
        }
        self.paths = {k: os.path.join(TARGET_DIR, v) for k, v in self.names.items()}

        self.host_build_task = EpicsBaseBuildTask(
            [
                self.src_path,
                self.paths["host_build"],
                self.paths["host_install"],
            ],
            None,
            deps=[self.repo.clone_task],
        )
        self.cross_build_task = EpicsBaseBuildTask(
            [
                self.paths["host_build"],
                self.paths["cross_build"],
                self.paths["cross_install"],
            ],
            self.cross_toolchain,
            deps=[self.host_build_task],
        )

    def tasks(self) -> dict[str, Task]:
        return {
            "clone": self.repo.clone_task,
            "build_host": self.host_build_task,
            "build_cross": self.cross_build_task,
        }
