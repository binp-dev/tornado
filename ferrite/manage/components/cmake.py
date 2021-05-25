from __future__ import annotations
import os
from utils.run import run
from manage.components.base import Component, Task, Context

class CmakeTask(Task):
    def __init__(self, owner: Cmake):
        super().__init__()
        self.owner = owner

class CmakeBuildTask(CmakeTask):
    def run(self, ctx: Context) -> bool:
        self.owner.configure(ctx)
        return self.owner.build(ctx)
    
    def artifacts(self) -> str[list]:
        return [self.owner.build_dir]

class CmakeTestTask(CmakeTask):
    def run(self, ctx: Context):
        self.owner.test(ctx)
    
    def dependencies(self) -> list[Task]:
        return [self.owner.build_task]

class Cmake(Component):
    def __init__(
        self,
        src_dir: str,
        build_dir: str,
        opt: list[str] = [],
        env: dict[str, str] = None,
    ):
        super().__init__()

        self.src_dir = src_dir
        self.build_dir = build_dir
        self.opt = opt
        self.env = env

        self.build_task = CmakeBuildTask(self)
        self.test_task = CmakeTestTask(self)

    def configure(self, ctx: Context, cvars: dict[str, str] = {}):
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
            quiet=ctx.capture,
        )

    # TODO: Detect that project is already built
    def build(self, ctx: Context, target=None, verbose=False) -> bool:
        run(
            [
                "cmake",
                "--build", self.build_dir,
                *(["--target", target] if target else []),
                "--parallel",
                *(["--verbose"] if verbose else []),
            ],
            cwd=self.build_dir,
            quiet=ctx.capture,
        )
        return True

    def test(self, ctx: Context):
        run(["ctest", "--verbose"], cwd=self.build_dir, quiet=ctx.capture)

    def tasks(self) -> dict[str, Task]:
        return {
            "build": self.build_task,
            "test": self.test_task,
        }
