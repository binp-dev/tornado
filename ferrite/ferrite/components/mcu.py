from __future__ import annotations
from typing import Dict, List, Optional

import shutil
from pathlib import Path, PurePosixPath

from ferrite.components.base import Artifact, Component, Task, Context, TaskWrapper
from ferrite.components.cmake import Cmake
from ferrite.components.toolchains import CrossToolchain, McuToolchainImx7, McuToolchainImx8mn, Toolchain
from ferrite.components.ipp import Ipp
from ferrite.components.freertos import Freertos
from ferrite.remote.tasks import RebootTask


class McuTask(Task):

    def __init__(self, owner: Mcu):
        super().__init__()
        self.owner = owner


class McuBuildTask(McuTask):

    def __init__(self, owner: Mcu):
        super().__init__(owner)

    def run(self, ctx: Context) -> None:
        # Workaround to disable cmake caching (incremental build is broken anyway)
        build_dir = self.owner.cmake.build_dir
        if build_dir.exists():
            shutil.rmtree(build_dir)

        self.owner.cmake.build_task.run(ctx)

    def dependencies(self) -> List[Task]:
        return [
            self.owner.toolchain.download_task,
            self.owner.freertos.clone_task,
            self.owner.ipp.generate_task,
        ]

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.cmake.build_dir)]


class McuDeployTask(McuTask):

    def __init__(self, owner: Mcu):
        super().__init__(owner)

    def dependencies(self) -> List[Task]:
        return [self.owner.tasks()["build"]]


class McuDeployTaskImx7(McuDeployTask):

    def __init__(self, owner: Mcu):
        super().__init__(owner)

    def run(self, ctx: Context) -> None:
        assert ctx.device is not None
        ctx.device.store(
            self.owner.cmake.build_dir / "release/m4image.bin",
            PurePosixPath("/m4image.bin"),
        )
        ctx.device.run(["bash", "-c", " && ".join([
            "mount /dev/mmcblk2p1 /mnt",
            "mv /m4image.bin /mnt",
            "umount /mnt",
        ])])


class McuDeployTaskImx8mn(McuDeployTask):

    def __init__(self, owner: Mcu):
        super().__init__(owner)

    def run(self, ctx: Context) -> None:
        assert ctx.device is not None
        ctx.device.store(
            self.owner.cmake.build_dir / "m7image.bin",
            PurePosixPath("/boot/m7image.bin"),
        )


class Mcu(Component):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: CrossToolchain,
        freertos: Freertos,
        ipp: Ipp,
    ):
        super().__init__()

        self.src_dir = source_dir / f"mcu/{toolchain.name}"
        self.toolchain = toolchain
        self.freertos = freertos
        self.ipp = ipp

        self.cmake = Cmake(
            self.src_dir,
            target_dir / f"mcu_{self.toolchain.name}",
            toolchain,
            opt=[
                "-DCMAKE_TOOLCHAIN_FILE={}".format(self.freertos.path / "tools/cmake_toolchain_files/armgcc.cmake"),
                "-DCMAKE_BUILD_TYPE=Release",
                *self.ipp.cmake_opts,
            ],
            env={
                "FREERTOS_DIR": str(self.freertos.path),
                "ARMGCC_DIR": str(self.toolchain.path),
            }
        )

        self.build_task = McuBuildTask(self)

        if isinstance(toolchain, McuToolchainImx7):
            self.deploy_task: McuDeployTask = McuDeployTaskImx7(self)
        elif isinstance(toolchain, McuToolchainImx8mn):
            self.deploy_task = McuDeployTaskImx8mn(self)
        else:
            raise Exception("Unknown toolchain class")

        self.deploy_and_reboot_task = TaskWrapper(RebootTask(), deps=[self.deploy_task])

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
            "deploy": self.deploy_task,
            "deploy_and_reboot": self.deploy_and_reboot_task,
        }
