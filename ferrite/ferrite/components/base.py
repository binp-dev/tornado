from __future__ import annotations
from typing import List, Optional
from ferrite.remote.base import Device


class Context:

    def __init__(self, device: Device = None, capture: bool = False):
        super().__init__()
        self.device = device
        self.capture = capture


class Task(object):

    def __init__(self) -> None:
        super().__init__()
        self._name = None

    def name(self) -> str:
        return self._name or str(self)

    def run(self, ctx: Context) -> bool:
        raise NotImplementedError

    def dependencies(self) -> List[Task]:
        return []

    def run_with_dependencies(self, ctx: Context) -> bool:
        ret = False
        deps = self.dependencies()
        assert isinstance(deps, list)

        for dep in deps:
            ret = dep.run_with_dependencies(ctx) or ret

        return self.run(ctx) or ret

    def artifacts(self) -> List[str]:
        return []


class FinalTask(Task):

    def __init__(self) -> None:
        super().__init__()


class TaskList(Task):

    def __init__(self, tasks: List[Task]) -> None:
        super().__init__()
        self.tasks = tasks

    def run(self, ctx: Context) -> bool:
        res = False
        for task in self.tasks:
            if task.run(ctx):
                res = True
        return res

    def dependencies(self) -> List[Task]:
        return [dep for task in self.tasks for dep in task.dependencies()]

    def artifacts(self) -> List[str]:
        return [art for task in self.tasks for art in task.artifacts()]


class TaskWrapper(Task):

    def __init__(self, inner: Optional[Task] = None, deps: List[Task] = []) -> None:
        super().__init__()
        self.inner = inner
        self.deps = deps

    def name(self) -> str:
        if self.inner is not None:
            return self.inner.name()
        else:
            return super().name()

    def run(self, ctx: Context) -> bool:
        if self.inner is not None:
            return self.inner.run(ctx)
        else:
            return False

    def dependencies(self) -> List[Task]:
        inner_deps = []
        if self.inner is not None:
            inner_deps = self.inner.dependencies()
        return inner_deps + self.deps

    def artifacts(self) -> List[str]:
        if self.inner is not None:
            return self.inner.artifacts()
        else:
            return []


class Component:

    def __init__(self) -> None:
        super().__init__()

    def tasks(self) -> dict[str, Task]:
        raise NotImplementedError
