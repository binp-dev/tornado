from __future__ import annotations
import os
from manage.paths import base_dir, target_dir
from manage.components.base import Component, Task
from manage.components.cmake import Cmake

class App(Component):
    def __init__(self, cross_toolchain):
        super().__init__()

        self.src_dir = os.path.join(base_dir, "app")
        self.cross_toolchain = cross_toolchain
        
        self.local_tests = Cmake(self.src_dir, os.path.join(target_dir, "app_local_tests"))

    def tasks(self) -> dict[str, Task]:
        return {
            "build_local_tests": self.local_tests.build_task,
            "run_local_tests": self.local_tests.test_task,
        }
