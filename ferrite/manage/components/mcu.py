from __future__ import annotations
import os
from manage.paths import BASE_DIR, TARGET_DIR
from manage.components.base import Component, Task, Context
from manage.components.cmake import Cmake

class McuTask(Task):
    def __init__(self, owner):
        super().__init__()
        self.owner = owner

class McuBuildTask(McuTask):
    def __init__(self, owner):
        super().__init__(owner)

    def run(self, ctx: Context) -> bool:
        return self.owner.cmake.build_task.run(ctx)
    
    def dependencies(self) -> list[Task]:
        return [
            self.owner.cross_toolchain.download_task,
            self.owner.freertos.clone_task,
        ]

class McuDeployTask(McuTask):
    def __init__(self, owner):
        super().__init__(owner)

class Mcu(Component):
    def __init__(self, freertos, cross_toolchain):
        super().__init__()

        self.src_dir = os.path.join(BASE_DIR, "mcu")
        self.freertos = freertos
        self.cross_toolchain = cross_toolchain
        
        self.cmake = Cmake(
            self.src_dir,
            os.path.join(TARGET_DIR, "mcu"),
            opt=[
                "-DCMAKE_TOOLCHAIN_FILE={}".format(os.path.join(
                    self.freertos.path,
                    "tools/cmake_toolchain_files/armgcc.cmake",
                )),
                "-DCMAKE_BUILD_TYPE=Release",
            ],
            env={
                "FREERTOS_DIR": self.freertos.path,
                "ARMGCC_DIR": self.cross_toolchain.path,
            }
        )

    def tasks(self) -> dict[str, Task]:
        return {
            "build": McuBuildTask(self),
        }
