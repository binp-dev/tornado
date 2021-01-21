from __future__ import annotations
import os
from manage.paths import BASE_DIR, TARGET_DIR
from manage.components.base import Component, Task
from manage.components.cmake import Cmake

class App(Component):
    def __init__(self, cross_toolchain=None):
        super().__init__()

        self.src_dir = os.path.join(BASE_DIR, "app")
        self.cross_toolchain = cross_toolchain
        
        self.host_tests = Cmake(self.src_dir, os.path.join(TARGET_DIR, "app_local_tests"))

    def tasks(self) -> dict[str, Task]:
        return {
            "build_host_tests": self.host_tests.build_task,
            "run_host_tests": self.host_tests.test_task,
        }
