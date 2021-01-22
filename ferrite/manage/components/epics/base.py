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
        install_dir: str,
        deps: str=[],
    ):
        super().__init__()
        self.src_dir = src_dir
        self.build_dir = build_dir
        self.install_dir = install_dir
        self.deps = deps

    def _configure(self):
        raise NotImplementedError

    def _variables(self) -> dict[str, str]:
        raise NotImplementedError

    def _make_done_file(self, path):
        with open(path, "w") as f:
            f.write("done")

    def _pre_build(self):
        pass
    def _post_build(self):
        pass

    def run(self, ctx: Context) -> bool:
        done_build = os.path.join(self.build_dir, "build.done")
        if not os.path.exists(self.build_dir):
            shutil.copytree(self.src_dir, self.build_dir)
            if os.path.exists(done_build):
                os.remove(done_build)

        if not os.path.exists(done_build):
            if os.path.exists(self.install_dir):
                shutil.rmtree(self.install_dir)
            os.makedirs(self.install_dir, exist_ok=True)

            self._pre_build()
            self._configure()
            run(
                [
                    "make",
                    "install",
                    "--jobs",
                    *[f"{k}={v}" for k, v in self._variables().items()],
                ],
                cwd=self.build_dir,
            )
            self._post_build()

            self._make_done_file(done_build)
            return True
        else:
            logging.info(f"{self.build_dir} is already built")
            return False

    def dependencies(self) -> list[Task]:
        return self.deps
