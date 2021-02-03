from __future__ import annotations
import os
import shutil
import logging
from utils.run import run
from manage.components.base import Task, Context

def epics_host_arch(epics_base_dir: str):
    return run([
        "perl",
        os.path.join(epics_base_dir, "src", "tools", "EpicsHostArch.pl"),
    ], capture=True, log=False).strip()

def epics_arch_by_target(target: str) -> str:
    if target.startswith("arm-linux-"):
        return "linux-arm"
    # TODO: Add some other archs
    raise Exception(f"Unknown target: {target}")

def epics_arch(epics_base_dir: str, target: str):
    if target is not None:
        return epics_arch_by_target(target)
    else:
        return epics_host_arch(epics_base_dir)

class EpicsBuildTask(Task):
    def __init__(
        self,
        src_dir: str,
        build_dir: str,
        install_dir: str,
        mk_target: str = None,
        deps: list[Task] = [],
    ):
        super().__init__()
        self.src_dir = src_dir
        self.build_dir = build_dir
        self.install_dir = install_dir
        self.mk_target = mk_target
        self.deps = deps

    def _configure(self):
        raise NotImplementedError

    def _install(self):
        raise NotImplementedError

    def run(self, ctx: Context) -> bool:
        shutil.copytree(
            self.src_dir, self.build_dir,
            dirs_exist_ok=True,
            ignore=shutil.ignore_patterns(".git")
        )
        
        logging.info(f"Configure {self.build_dir}")
        self._configure()

        logging.info(f"Build {self.build_dir}")
        run([
            "make",
            "--jobs",
            *([self.mk_target] if self.mk_target is not None else []),
        ], cwd=self.build_dir)
        
        logging.info(f"Install {self.build_dir} to {self.install_dir}")
        os.makedirs(self.install_dir, exist_ok=True)
        self._install()
        
        return True

    def dependencies(self) -> list[Task]:
        return self.deps
