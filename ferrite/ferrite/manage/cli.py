from __future__ import annotations
from typing import Dict, List, Optional

import argparse
from dataclasses import dataclass
from colorama import init as colorama_init, Fore, Style

from ferrite.components.base import Context, Task, Component
from ferrite.manage.runner import Runner
from ferrite.remote.ssh import SshDevice

import logging


def _make_task_tree(tasks: List[str]) -> List[str]:
    output: List[str] = []
    groups: Dict[str, List[str] | None] = {}
    for task in tasks:
        spl = task.split(".", 1)
        if len(spl) == 1:
            key = spl[0]
            assert key not in groups
            groups[key] = None
        elif len(spl) == 2:
            key, value = spl
            if key in groups:
                values = groups[key]
                assert values is not None
                values.append(value)
            else:
                groups[key] = [value]

    for key, values in sorted(groups.items(), key=lambda x: x[0]):
        if values is None:
            output.append(f"{Style.BRIGHT}{key}{Style.NORMAL}")
        else:
            assert len(values) > 0
            subtree = _make_task_tree(values)
            output.append(f"{Style.BRIGHT}{key}.{subtree[0]}{Style.NORMAL}")
            output.extend([f"{Style.DIM}{key}.{Style.NORMAL}{value}" for value in subtree[1:]])

    return output


def _available_tasks_text(comp: Component) -> str:
    return "\n".join([
        "Available tasks:",
        *[(" " * 2) + task for task in _make_task_tree(list(comp.tasks().keys()))],
    ])


def add_parser_args(parser: argparse.ArgumentParser, comp: Component) -> None:
    parser.formatter_class = argparse.RawTextHelpFormatter

    parser.add_argument(
        "task",
        type=str,
        metavar="<task>",
        help="\n".join([
            "Task you want to run.",
            _available_tasks_text(comp),
        ]),
    )
    parser.add_argument(
        "--no-deps",
        action="store_true",
        help="Run only specified task without dependencies.",
    )
    parser.add_argument(
        "--device",
        type=str,
        metavar="<address>[:port]",
        default=None,
        help="\n".join([
            "Device to deploy and run tests.", "Requirements:",
            "+ Debian Linux running on the device (another distros are not tested).",
            "+ SSH server running on the device on the specified port (or 22 if the port is not specified).",
            "+ Possibility to log in to the device via SSH by user 'root' without password (e.g. using public key)."
        ])
    )
    parser.add_argument(
        "--no-capture",
        action="store_true",
        help="Display task stdout.",
    )
    parser.add_argument(
        "--update",
        action="store_true",
        help="Update external dependencies.",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        metavar="<N>",
        default=None,
        help="Number of parallel process to build. By default automatically determined value is used.",
    )


class ReadRunParamsError(RuntimeError):
    pass


@dataclass
class RunParams:
    task: Task
    context: Context
    no_deps: bool = False


def _find_task_by_args(comp: Component, args: argparse.Namespace) -> Task:
    task_name = args.task
    try:
        task = comp.tasks()[task_name]
    except KeyError:
        raise ReadRunParamsError("\n".join([f"Unknown task '{task_name}'.", _available_tasks_text(comp)]))

    return task


def _make_context_from_args(args: argparse.Namespace) -> Context:
    device = None
    if args.device:
        device = SshDevice(args.device)

    return Context(
        device=device,
        capture=not args.no_capture,
        update=args.update,
        jobs=args.jobs,
    )


def read_run_params(args: argparse.Namespace, comp: Component) -> RunParams:
    try:
        task = _find_task_by_args(comp, args)
    except ReadRunParamsError as e:
        print(e)
        exit(1)

    context = _make_context_from_args(args)

    return RunParams(task, context, no_deps=args.no_deps)


def _prepare_for_run(params: RunParams) -> None:
    colorama_init()


def setup_logging(params: RunParams, modules: List[str] = ["ferrite"]) -> None:
    logging.basicConfig(format='[%(levelname)s] %(message)s', level=logging.WARNING, force=True)
    if not params.context.capture:
        for mod in modules:
            logging.getLogger(mod).setLevel(logging.DEBUG)


def run_with_params(params: RunParams) -> None:
    _prepare_for_run(params)
    Runner(params.task).run(params.context, no_deps=params.no_deps)
