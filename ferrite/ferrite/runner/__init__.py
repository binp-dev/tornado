from __future__ import annotations

from graphlib import TopologicalSorter

from ferrite.utils.path import TargetPath
from ferrite.components.base import Context, Task

from .isolation import isolated, with_artifacts
from .print import with_info


class Runner:

    def __init__(self, task: Task, no_deps: bool = False) -> None:
        self.hideout_dir = TargetPath(".hideout")

        if no_deps:
            self.sequence = [task]
        else:
            self.sequence = list(TopologicalSorter(task.graph()).static_order())

    def run(self, context: Context) -> None:
        context.target_path.mkdir(exist_ok=True)

        if not context.hide_artifacts:
            for task in self.sequence:
                with with_info(task, context):
                    task.run(context)
        else:
            hideout_path = context.target_path / self.hideout_dir
            with isolated(context.target_path, hideout_path):
                for task in self.sequence:
                    with with_info(task, context):
                        with with_artifacts(context.target_path, hideout_path, task):
                            task.run(context)
