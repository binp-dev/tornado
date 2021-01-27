from __future__ import annotations
import os
from manage.components.base import Component, Task, Context
from manage.utils.run import run

class CmakeTask(Task):
    def __init__(self, owner: Cmake):
        super().__init__()
        self.owner = owner

class CmakeBuildTask(CmakeTask):
    def run(self, ctx: Context) -> bool:
        self.owner.configure()
        return self.owner.build()

class CmakeTestTask(CmakeTask):
    def run(self, ctx: Context):
        self.owner.test()
    
    def dependencies(self) -> list[Task]:
        return [self.owner.build_task]

class Cmake(Component):
    def __init__(
        self,
        src_dir: str,
        build_dir: str,
        opt: list[str]=[],
        env: dict[str, str]=None,
    ):
        super().__init__()

        self.src_dir = src_dir
        self.build_dir = build_dir
        self.opt = opt
        self.env = env

        self.build_task = CmakeBuildTask(self)
        self.test_task = CmakeTestTask(self)

    def configure(self, cvars: dict[str, str] = {}):
        os.makedirs(self.build_dir, exist_ok=True)
        run(
            [
                "cmake",
                *self.opt,
                *[f"-D{k}={v}" for k, v in cvars.items()],
                self.src_dir,
            ],
            cwd=self.build_dir,
            add_env=self.env,
        )

    # TODO: Detect that project is already built
    def build(self, target=None) -> bool:
        run(
            [
                "cmake",
                "--build", self.build_dir,
                *(["--target", target] if target else []),
                "--parallel",
            ],
            cwd=self.build_dir,
        )
        return True

    def test(self):
        run(["ctest", "--verbose"], cwd=self.build_dir)

    def tasks(self) -> dict[str, Task]:
        return {
            "build": self.build_task,
            "test": self.test_task,
        }
