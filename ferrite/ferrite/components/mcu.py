from __future__ import annotations
from dataclasses import dataclass, field
from typing import Dict, List

import shutil
from pathlib import Path

from ferrite.utils.path import TargetPath
from ferrite.components.base import Task, OwnedTask, Context, TaskWrapper
from ferrite.components.cmake import Cmake
from ferrite.components.compiler import GccCross
from ferrite.components.freertos import Freertos
from ferrite.remote.base import Device
from ferrite.remote.tasks import RebootTask


class McuDeployer:

    def deploy(self, build_dir: Path, device: Device) -> None:
        raise NotImplementedError()


class McuBase(Cmake):

    def configure(self, ctx: Context) -> None:
        build_path = ctx.target_path / self.build_dir

        # Workaround to disable cmake caching (incremental build is broken anyway)
        if build_path.exists():
            shutil.rmtree(build_path)

        super().configure(ctx)

    def __init__(
        self,
        src_dir: Path,
        build_dir: TargetPath,
        cc: GccCross,
        freertos: Freertos,
        deployer: McuDeployer,
        target: str,
        deps: List[Task] = [],
    ):
        super().__init__(
            src_dir,
            build_dir,
            cc,
            target=target,
            deps=[
                freertos.clone_task,
                *deps,
            ],
        )
        self.freertos = freertos

        self.deploy_task = _DeployTask(self, deployer)
        self.deploy_and_reboot_task = TaskWrapper(RebootTask(), deps=[self.deploy_task])

    def env(self, ctx: Context) -> Dict[str, str]:
        assert isinstance(self.cc, GccCross)
        return {
            **super().env(ctx),
            "FREERTOS_DIR": str(ctx.target_path / self.freertos.path),
            "ARMGCC_DIR": str(ctx.target_path / self.cc.path),
        }

    def opt(self, ctx: Context) -> List[str]:
        return [
            *super().opt(ctx),
            f"-DCMAKE_TOOLCHAIN_FILE={ctx.target_path / self.freertos.path / 'tools/cmake_toolchain_files/armgcc.cmake'}",
            "-DCMAKE_BUILD_TYPE=Release",
        ]


@dataclass
class _DeployTask(OwnedTask[McuBase]):
    deployer: McuDeployer

    def run(self, ctx: Context) -> None:
        assert ctx.device is not None
        self.deployer.deploy(ctx.target_path / self.owner.build_dir, ctx.device)

    def dependencies(self) -> List[Task]:
        return [self.owner.build_task]
