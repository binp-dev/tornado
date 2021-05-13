from __future__ import annotations
import os
import shutil
from manage.paths import BASE_DIR, TARGET_DIR
from manage.components.base import Component, Task, Context, TaskWrapper
from manage.components.cmake import Cmake
from manage.remote.tasks import RebootTask

class McuTask(Task):
    def __init__(self, owner):
        super().__init__()
        self.owner = owner

class McuBuildTask(McuTask):
    def __init__(self, owner):
        super().__init__(owner)

    def run(self, ctx: Context) -> bool:
        # Workaround to disable cmake caching (incremental build is broken anyway)
        build_dir = self.owner.cmake.build_dir
        if os.path.exists(build_dir):
            shutil.rmtree(build_dir)

        return self.owner.cmake.build_task.run(ctx)

    def dependencies(self) -> list[Task]:
        return [
            self.owner.toolchain.download_task,
            self.owner.freertos.clone_task,
        ]

    def artifacts(self) -> str[list]:
        return [self.owner.cmake.build_dir]

class McuDeployTask(McuTask):
    def __init__(self, owner):
        super().__init__(owner)

    def run(self, ctx: Context) -> bool:
        assert ctx.device is not None
        ctx.device.store(
            os.path.join(self.owner.cmake.build_dir, "release/m4image.bin"),
            "/m4image.bin",
        )
        ctx.device.run(["bash", "-c", " && ".join([
            "mount /dev/mmcblk2p1 /mnt",
            "mv /m4image.bin /mnt",
            "umount /mnt",
        ])])

    def dependencies(self) -> list[Task]:
        return [self.owner.tasks()["build"]]

class Mcu(Component):
    def __init__(self, freertos, toolchain):
        super().__init__()

        self.src_dir = os.path.join(BASE_DIR, f"mcu/{toolchain.name}")
        self.freertos = freertos
        self.toolchain = toolchain

        self.cmake = Cmake(
            self.src_dir,
            os.path.join(TARGET_DIR, f"mcu_{self.toolchain.name}"),
            opt=[
                "-DCMAKE_TOOLCHAIN_FILE={}".format(os.path.join(
                    self.freertos.path,
                    "tools/cmake_toolchain_files/armgcc.cmake",
                )),
                "-DCMAKE_BUILD_TYPE=Release",
            ],
            env={
                "COMMON_DIR": os.path.join(BASE_DIR, "common"),
                "FREERTOS_DIR": self.freertos.path,
                "ARMGCC_DIR": self.toolchain.path,
            }
        )
        self.build_task = McuBuildTask(self)
        self.deploy_task = McuDeployTask(self)
        self.deploy_and_reboot_task = TaskWrapper(RebootTask(), deps=[self.deploy_task])

    def tasks(self) -> dict[str, Task]:
        return {
            "build": self.build_task,
            "deploy": self.deploy_task,
            "deploy_and_reboot": self.deploy_and_reboot_task,
        }
