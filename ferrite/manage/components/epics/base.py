from __future__ import annotations
import os
import shutil
import logging
from manage.components.base import Task, Context
from manage.utils.run import run

def epics_host_arch(epics_base_dir: str):
    return run([
        "perl",
        os.path.join(epics_base_dir, "src", "tools", "EpicsHostArch.pl"),
    ], capture=True).strip()

def epics_arch_by_target(target: str) -> str:
    if target.startswith("arm-linux-"):
        return "linux-arm"
    # TODO: Add some other archs
    return None

class EpicsBuildTask(Task):
    def __init__(
        self,
        src_dir: str,
        build_dir: str,
        deps: list[Task],
    ):
        super().__init__()
        self.src_dir = src_dir
        self.build_dir = build_dir
        self.deps = deps

    def _configure(self):
        raise NotImplementedError

    def run(self, ctx: Context) -> bool:
        shutil.copytree(
            self.src_dir, self.build_dir,
            dirs_exist_ok=True,
            ignore=shutil.ignore_patterns(".git")
        )
        self._configure()
        run(["make", "--jobs"], cwd=self.build_dir)
        return True

    def dependencies(self) -> list[Task]:
        return self.deps
