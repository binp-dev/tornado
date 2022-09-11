from __future__ import annotations
from typing import Dict, Set

from pathlib import Path
from graphlib import TopologicalSorter

from colorama import Fore, Style

from ferrite.components.base import Context, Task

from .isolation import isolated, with_artifacts
from .print import with_info


class Runner:

    def __init__(self, target_dir: Path, task: Task, no_deps: bool = False) -> None:
        self.target_dir = target_dir
        self.hideout_dir = target_dir / ".hideout"

        if no_deps:
            self.sequence = [task]
        else:
            self.sequence = list(TopologicalSorter(task.graph()).static_order())

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
