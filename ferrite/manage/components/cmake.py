from __future__ import annotations
import os
from manage.components.base import Component, Task, Context
from manage.utils.run import run

class CmakeTask(Task):
    def __init__(self, owner: Cmake):
        super().__init__()
        self.owner = owner

class CmakeBuildTask(CmakeTask):
    # TODO: Detect that project is already built
    def run(self, ctx: Context) -> bool:
        os.makedirs(self.owner.build_dir, exist_ok=True)
        run(
            ["cmake", *self.owner.opt, self.owner.src_dir],
            cwd=self.owner.build_dir,
            add_env=self.owner.env,
        )
        run(["cmake", "--build", self.owner.build_dir], cwd=self.owner.build_dir)
        return True

class CmakeTestTask(CmakeTask):
    def run(self, ctx: Context):
        run(["ctest", "--verbose"], cwd=self.owner.build_dir)
    
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

    def tasks(self) -> dict[str, Task]:
        return {
            "build": self.build_task,
            "test": self.test_task,
        }
