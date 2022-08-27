from __future__ import annotations
from typing import Any, Callable, Dict, List, Optional

from random import getrandbits
from pathlib import Path
from dataclasses import dataclass

from ferrite.remote.base import Device


@dataclass
class Context:
    device: Optional[Device] = None
    capture: bool = False
    jobs: Optional[int] = None


@dataclass
class Artifact:
    path: Path
    cached: bool = False


@dataclass(eq=False)
class Task:

    def __post_init__(self) -> None:
        self._name: Optional[str] = None

    def name(self) -> str:
        return self._name or f"<{type(self).__qualname__}>({hash(self):x})"

    def run(self, ctx: Context) -> None:
        raise NotImplementedError()

    def dependencies(self) -> List[Task]:
        return []

    def run_with_dependencies(self, ctx: Context) -> None:
        deps = self.dependencies()
        assert isinstance(deps, list)

        for dep in deps:
            dep.run_with_dependencies(ctx)

        self.run(ctx)

    def artifacts(self) -> List[Artifact]:
        return []


class EmptyTask(Task):

    def run(self, ctx: Context) -> None:
        pass


@dataclass
class CallTask(Task):
    func: Callable[[], None]

    def run(self, ctx: Context) -> None:
        self.func()


class TaskList(Task):

    def __init__(self, tasks: List[Task]) -> None:
        super().__init__()
        self.tasks = tasks

    def run(self, ctx: Context) -> None:
        pass

    def dependencies(self) -> List[Task]:
        return [dep for task in self.tasks for dep in task.dependencies()]

    def artifacts(self) -> List[Artifact]:
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

    def run(self, ctx: Context) -> None:
        if self.inner is not None:
            self.inner.run(ctx)

    def dependencies(self) -> List[Task]:
        inner_deps = []
        if self.inner is not None:
            inner_deps = self.inner.dependencies()
        return inner_deps + self.deps

    def artifacts(self) -> List[Artifact]:
        return [
            *(self.inner.artifacts() if self.inner is not None else []),
            *[art for task in self.deps for art in task.artifacts()]
        ]


class Component:

    def tasks(self) -> Dict[str, Task]:
        raise NotImplementedError()

    def _update_names(self) -> None:
        for task_name, task in self.tasks().items():
            if hasattr(task, "_name") and task._name is not None:
                raise RuntimeError(f"Task has multiple names: '{task._name}' and '{task_name}'")
            task._name = f"{task_name}"


@dataclass
class DictComponent(Component):
    task_dict: Dict[str, Task]

    def tasks(self) -> Dict[str, Task]:
        return self.task_dict


class ComponentGroup(Component):

    def components(self) -> Dict[str, Component]:
        raise NotImplementedError()

    def tasks(self) -> Dict[str, Task]:
        tasks: Dict[str, Task] = {}
        for comp_name, comp in self.components().items():
            for task_name, task in comp.tasks().items():
                key = f"{comp_name}.{task_name}"
                assert key not in tasks
                tasks[key] = task
        return tasks
