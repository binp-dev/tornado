from __future__ import annotations
from typing import Dict, Optional, Set

from graphlib import TopologicalSorter

from colorama import Fore, Style

from ferrite.components.base import Context, Task


def _print_title(text: str, style: Optional[str] = None, end: bool = True) -> None:
    if style is not None:
        text = style + text + Style.RESET_ALL
    print(text, flush=True, end=("" if not end else None))


def _run_task_without_deps(task: Task, context: Context) -> None:
    if context.capture:
        _print_title(f"{task.name()} ... ", end=False)
    else:
        _print_title(f"\nTask '{task.name()}' started ...", Style.BRIGHT)

    try:
        task.run(context)
    except:
        if context.capture:
            _print_title(f"FAIL", Fore.RED)
        else:
            _print_title(f"Task '{task.name()}' FAILED:", Style.BRIGHT + Fore.RED)
        raise
    else:
        if context.capture:
            _print_title(f"ok", Fore.GREEN)
        else:
            _print_title(f"Task '{task.name()}' successfully completed", Style.BRIGHT + Fore.GREEN)


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

    def __init__(self, task: Task) -> None:
        self.task = task

        graph: Dict[Task, Set[Task]] = {}
        _fill_graph(graph, task)
        self.sequence = list(TopologicalSorter(graph).static_order())

    def run(self, context: Context, no_deps: bool = False) -> None:
        if no_deps:
            _run_task_without_deps(self.task, context)
            return

        for task in self.sequence:
            _run_task_without_deps(task, context)
