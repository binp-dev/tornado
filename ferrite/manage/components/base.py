from __future__ import annotations
import logging
from manage.remote import Device

class Context(object):
    def __init__(self, device: Device = None):
        super().__init__()
        self.device = device

class Task(object):
    def __init__(self):
        super().__init__()

    def run(self, ctx: Context) -> bool:
        raise NotImplementedError

    def dependencies(self) -> list[Task]:
        return []

    def run_with_dependencies(self, ctx: Context) -> bool:
        ret = False
        deps = self.dependencies()
        assert isinstance(deps, list)

        for dep in deps:
            ret = dep.run_with_dependencies(ctx) or ret

        return self.run(ctx) or ret

class Component(object):
    def __init__(self):
        super().__init__()

    def tasks(self) -> dict[str, Task]:
        raise NotImplementedError
