from __future__ import annotations
from typing import Dict, Set

from pathlib import Path
from graphlib import TopologicalSorter

from colorama import Fore, Style

from ferrite.components.base import Context, Task

from .isolation import isolated, with_artifacts
from .print import with_info


def _fill_graph(graph: Dict[Task, Set[Task]], task: Task) -> None:
    if task not in graph:
        deps = task.dependencies()
        graph[task] = set(deps)
        for dep in deps:
            _fill_graph(graph, dep)
    else:
        # Check that task dependencies are the same.
        assert len(graph[task].symmetric_difference(set(task.dependencies()))) == 0


class Runner:

    def __init__(self, target_dir: Path, task: Task, no_deps: bool = False) -> None:
        self.target_dir = target_dir
        self.hideout_dir = target_dir / ".hideout"

        if no_deps:
            self.sequence = [task]
        else:
            graph: Dict[Task, Set[Task]] = {}
            _fill_graph(graph, task)
            self.sequence = list(TopologicalSorter(graph).static_order())

    def run(self, context: Context) -> None:
        if not context.hide_artifacts:
            for task in self.sequence:
                with with_info(task, context):
                    task.run(context)
        else:
            with isolated(self.target_dir, self.hideout_dir):
                for task in self.sequence:
                    with with_info(task, context):
                        with with_artifacts(self.target_dir, self.hideout_dir, task):
                            task.run(context)
