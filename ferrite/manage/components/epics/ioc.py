from __future__ import annotations
import os
from manage.components.base import Component, Task
from manage.components.git import Repo
from manage.components.toolchains import Toolchain
from manage.utils.files import substitute
from manage.paths import BASE_DIR, TARGET_DIR
from .base import EpicsBuildTask, epics_arch_by_target
from .epics_base import EpicsBase

class IocBuildTask(EpicsBuildTask):
    def __init__(self, base_args, epics_base_dir: str, toolchain: Toolchain, **kwargs):
        super().__init__(*base_args, **kwargs)
        self.epics_base_dir = epics_base_dir
        self.toolchain = toolchain

    def _configure(self):
        pass

    def _variables(self) -> dict[str, str]:
        v = {
            "EPICS_BASE": self.epics_base_dir,
            "INSTALL_LOCATION": self.install_dir,
            #"USR_CFLAGS": "",
            #"USR_CXXFLAGS": "",
            #"USR_LDFLAGS": "",
            #"LIB_SYS_LIBS": "",
        }
        if self.toolchain:
            v["CROSS_COMPILER_TARGET_ARCHS"] = epics_arch_by_target(self.toolchain.arch)
        return v

    def _post_build(self):
        shutil.copytree(
            os.path.join(self.build_dir, "iocBoot"),
            os.path.join(self.install_dir, "iocBoot"),
        )

class Ioc(Component):
    def __init__(self, name: str, path: str, epics_base: EpicsBase, cross_toolchain: Toolchain):
        super().__init__()

        self.name = name
        self.src_path = path
        self.epics_base = epics_base
        self.cross_toolchain = cross_toolchain

        self.names = {
            "host_build":    f"{self.name}_host_build",
            "cross_build":   f"{self.name}_cross_build",
            "host_install":  f"{self.name}_host_install",
            "cross_install": f"{self.name}_cross_install",
        }
        self.paths = {k: os.path.join(TARGET_DIR, v) for k, v in self.names.items()}

        self.host_build_task = IocBuildTask(
            [
                self.src_path,
                self.paths["host_build"],
                self.paths["host_install"],
            ],
            self.epics_base.paths["host_install"],
            None,
            deps=[self.epics_base.host_build_task],
        )
        self.cross_build_task = IocBuildTask(
            [
                self.paths["host_build"],
                self.paths["cross_build"],
                self.paths["cross_install"],
            ],
            self.epics_base.paths["cross_install"],
            self.cross_toolchain.path,
            deps=[self.epics_base.cross_build_task],
        )

    def tasks(self) -> dict[str, Task]:
        return {
            "build_host": self.host_build_task,
            "build_cross": self.cross_build_task,
        }

class AppIoc(Ioc):
    def __init__(
        self,
        epics_base: EpicsBase,
        cross_toolchain: Toolchain,
    ):
        super().__init__(
            "ioc",
            os.path.join(BASE_DIR, "ioc"),
            epics_base,
            cross_toolchain,
        )
