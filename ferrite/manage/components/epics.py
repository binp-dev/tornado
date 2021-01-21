from __future__ import annotations
import os
import shutil
import logging
from manage.components.base import Component, Task, TaskArgs
from manage.components.git import Repo
from manage.utils.run import run, RunError
from manage.utils.files import substitute
from manage.paths import TARGET_DIR

def _configure_common(base: str):
    flags = "-std=c++17"
    usr_vars = [
        #("USR_CFLAGS", ""),
        ("USR_CXXFLAGS", flags),
        #("USR_CPPFLAGS", ""),
    ]
    rules = [(r'^([ \t]*{}[ \t]*=[ \t]*)[^\n]*$'.format(v), r'\1{}'.format(f)) for v, f in usr_vars]
    substitute(rules, os.path.join(base, "configure/CONFIG_COMMON"))

# TODO: Detect host arch
def _configure_toolchain(base: str, toolchain: str):
    if toolchain is None:
        substitute([
            (r'^([ \t]*CROSS_COMPILER_TARGET_ARCHS[ \t]*=[ \t]*)[^\n]*$', r'\1'),
        ], os.path.join(base, "configure/CONFIG_SITE"))
        
    else:
        substitute([
            (r'^([ \t]*CROSS_COMPILER_TARGET_ARCHS[ \t]*=[ \t]*)[^\n]*$', r'\1linux-arm'),
        ], os.path.join(base, "configure/CONFIG_SITE"))

        substitute([
            (r'^([ \t]*GNU_TARGET[ \t]*=[ \t]*)[^\n]*$', r'\1arm-linux-gnueabihf'),
            (r'^([ \t]*GNU_DIR[ \t]*=[ \t]*)[^\n]*$', r'\1{}'.format(toolchain)),
        ], os.path.join(base, "configure/os/CONFIG_SITE.linux-x86.linux-arm"))

def _configure_install(base: str, install: str):
    if install is None:
        substitute([
            (r'^[ \t]*#*([ \t]*INSTALL_LOCATION[ \t]*=[ \t]*)[^\n]*$', r'#\1'),
        ], os.path.join(base, "configure/CONFIG_SITE"))

    else:
        substitute([
            (r'^[ \t]*#*([ \t]*INSTALL_LOCATION[ \t]*=[ \t]*)[^\n]*$', r'\1{}'.format(install)),
        ], os.path.join(base, "configure/CONFIG_SITE"))

def _configure(base: str, toolchain: str, install: str):
    _configure_common(base)
    _configure_toolchain(base, toolchain)
    _configure_install(base, install)

def _make_done_file(path):
    with open(path, "w") as f:
        f.write("done")

class EpicsTask(Task):
    def __init__(
        self,
        src_dir: str,
        build_dir: str,
        install_dir: str,
        toolchain_dir: str,
        dependencies: str,
    ):
        super().__init__()
        self.src_dir = src_dir
        self.build_dir = build_dir
        self.install_dir = install_dir
        self.toolchain_dir = toolchain_dir
        self.dependencies = dependencies

    def _run_deps(self, args):
        for dep in self.dependencies:
            dep.run(args)

class EpicsBuildTask(EpicsTask):
    def run(self, args: TaskArgs):
        self._run_deps(args)

        done_build = os.path.join(self.build_dir, "build.done")
        if not os.path.exists(self.build_dir):
            shutil.copytree(self.src_dir, self.build_dir)
            if os.path.exists(done_build):
                os.remove(done_build)
        
        if not os.path.exists(done_build):
            if os.path.exists(self.install_dir):
                shutil.rmtree(self.install_dir)
            os.makedirs(self.install_dir, exist_ok=True)

            _configure(self.build_dir, self.toolchain_dir, self.install_dir)
            run(["make", "install"], cwd=self.build_dir)
            _make_done_file(done_build)
        else:
            logging.info(f"{self.build_dir} is already built")

class EpicsBase(Component):
    def __init__(self, cross_toolchain=None):
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

        self.host_build_task = EpicsBuildTask(
            self.src_path,
            self.paths["host_build"],
            self.paths["host_install"],
            None,
            [self.repo.clone_task],
        )
        self.cross_build_task = EpicsBuildTask(
            self.paths["host_build"],
            self.paths["cross_build"],
            self.paths["cross_install"],
            self.cross_toolchain.path,
            [self.host_build_task],
        )

    def tasks(self) -> dict[str, Task]:
        return {
            "clone": self.repo.clone_task,
            "build_host": self.host_build_task,
            "build_cross": self.cross_build_task,
        }
