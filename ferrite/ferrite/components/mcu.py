from __future__ import annotations
from dataclasses import dataclass, field
from typing import Dict, List

import shutil
from pathlib import Path

from ferrite.components.base import Artifact, Component, Task, Context, TaskWrapper
from ferrite.components.cmake import Cmake
from ferrite.components.toolchain import CrossToolchain
from ferrite.components.freertos import Freertos
from ferrite.remote.base import Device
from ferrite.remote.tasks import RebootTask


class McuDeployer:

    def deploy(self, build_dir: Path, device: Device) -> None:
        raise NotImplementedError()


class McuBase(Cmake):

    @dataclass
    class DeployTask(Task):
        owner: McuBase
        deployer: McuDeployer

        def run(self, ctx: Context) -> None:
            assert ctx.device is not None
            self.deployer.deploy(self.owner.build_dir, ctx.device)

        def dependencies(self) -> List[Task]:
            return [self.owner.build_task]

    def configure(self, ctx: Context) -> None:
        # Workaround to disable cmake caching (incremental build is broken anyway)
        if self.build_dir.exists():
            shutil.rmtree(self.build_dir)

        super().configure(ctx)

    def __init__(
        self,
        name: str,
        src_dir: Path,
        target_dir: Path,
        toolchain: CrossToolchain,
        freertos: Freertos,
        deployer: McuDeployer,
        target: str,
        opts: List[str] = [],
        envs: Dict[str, str] = {},
        deps: List[Task] = [],
    ):
        toolchain = toolchain

        super().__init__(
            src_dir,
            target_dir / name,
            toolchain,
            target=target,
            opts=[
                "-DCMAKE_TOOLCHAIN_FILE={}".format(freertos.path / "tools/cmake_toolchain_files/armgcc.cmake"),
                "-DCMAKE_BUILD_TYPE=Release",
                *opts,
            ],
            envs={
                "FREERTOS_DIR": str(freertos.path),
                "ARMGCC_DIR": str(toolchain.path),
                **envs,
            },
            deps=[
                freertos.clone_task,
                *deps,
            ],
        )
        self.freertos = freertos

        self.deploy_task = self.DeployTask(self, deployer)
        self.deploy_and_reboot_task = TaskWrapper(RebootTask(), deps=[self.deploy_task])

    def tasks(self) -> Dict[str, Task]:
        tasks = super().tasks()
        tasks.update({
            "deploy": self.deploy_task,
            "deploy_and_reboot": self.deploy_and_reboot_task,
        })
        return tasks
