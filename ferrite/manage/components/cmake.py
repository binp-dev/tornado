from __future__ import annotations
import os
from manage.components.base import Component
from manage.tasks.base import Task
from manage.utils.run import run

class CmakeTask(Task):
    def __init__(self, owner: Cmake):
        super().__init__()
        self.owner = owner

class CmakeBuildTask(CmakeTask):
    def run(self, args: dict[str, str]):
        os.makedirs(self.owner.build_dir, exist_ok=True)
        run(["cmake", self.owner.src_dir], cwd=self.owner.build_dir)
        run(["cmake", "--build", self.owner.build_dir], cwd=self.owner.build_dir)

class CmakeTestTask(CmakeTask):
    def dependencies(self) -> list[Task]:
        return [self.owner.build_task]

    def run(self, args: dict[str, str]):
        self.owner.build_task.run(args)
        run(["ctest", "--verbose"], cwd=self.owner.build_dir)

class Cmake(Component):
    def __init__(self, src_dir: str, build_dir: str):
        super().__init__()

        self.src_dir = src_dir
        self.build_dir = build_dir

        self.build_task = CmakeBuildTask(self)
        self.test_task = CmakeTestTask(self)

    def tasks(self) -> dict[str, Task]:
        return {
            "build": self.build_task,
            "test": self.test_task,
        }
