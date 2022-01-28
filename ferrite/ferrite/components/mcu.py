from __future__ import annotations
from dataclasses import dataclass, field
from typing import Dict, List

import shutil
from pathlib import Path

from ferrite.components.base import Artifact, Component, Task, Context, TaskWrapper
from ferrite.components.cmake import Cmake
from ferrite.components.toolchain import CrossToolchain
from ferrite.components.ipp import Ipp
from ferrite.components.freertos import Freertos
from ferrite.remote.base import Device
from ferrite.remote.tasks import RebootTask


class McuTask(Task):

    def __init__(self, owner: Mcu):
        super().__init__()
        self.owner = owner


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
        source_dir: Path,
        target_dir: Path,
        toolchain: CrossToolchain,
        freertos: Freertos,
        deployer: McuDeployer,
        opts: List[str] = [],
        envs: Dict[str, str] = {},
        deps: List[Task] = [],
    ):
        src_dir = source_dir / f"mcu/{toolchain.name}"
        toolchain = toolchain

        super().__init__(
            src_dir,
            target_dir / f"mcu_{toolchain.name}",
            toolchain,
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


class Mcu(McuBase):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: CrossToolchain,
        freertos: Freertos,
        deployer: McuDeployer,
        ipp: Ipp,
    ):
        super().__init__(
            source_dir,
            target_dir,
            toolchain,
            freertos,
            deployer,
            opts=[f"-DIPP_GENERATED={ipp.gen_dir}"],
            deps=[ipp.generate_task],
        )
        self.ipp = ipp
