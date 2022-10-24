from __future__ import annotations
from typing import Dict, List, Optional

import argparse
from dataclasses import dataclass
from pathlib import Path
from colorama import init as colorama_init, Fore, Style

from ferrite.components.base import Context, Task, Component
from ferrite.utils.log import LogLevel
from ferrite.runner import Runner
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
        "-t",
        "--target-dir",
        type=str,
        default="target",
        help="Path to directory to place build artifacts. ('$PWD/target' by default)",
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
            "Device to deploy and run tests.",
            "Requirements:",
            "+ Debian Linux running on the device (another distros are not tested).",
            "+ SSH server running on the device on the specified port (or 22 if the port is not specified).",
            "+ Possibility to log in to the device via SSH by user 'root' without password (e.g. using public key).",
        ])
    )
    parser.add_argument(
        "--update",
        action="store_true",
        help="Update external dependencies (toolchains, locked dependencies, etc.).",
    )
    parser.add_argument(
        "--local",
        action="store_true",
        help="Store cache locally (for rustup, cargo, etc.).",
    )
    parser.add_argument(
        "--hide-artifacts",
        action="store_true",
        help="Hide artifacts which are unused by task. Useful to test task dependencies list.",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        metavar="<N>",
        default=None,
        help="Number of parallel process to build. By default automatically determined value is used.",
    )
    parser.add_argument(
        "--log-level",
        type=int,
        choices=[int(v) for v in LogLevel],
        default=None,
        help="\n".join([
            "Set log level.",
            "  0 - Trace",
            "  1 - Debug",
            "  2 - Info",
            "  3 - Warning (since this level task output is captured and displayed only in case of error)",
            "  4 - Error",
            "Default value is 3 (warning).",
        ]),
    )
    parser.add_argument(
        "--no-capture",
        action="store_true",
        help="[DEPRECATED] The same as `--verbosilty=2`.",
    )


class ReadRunParamsError(RuntimeError):
    pass


@dataclass
class RunParams:
    task: Task
    context: Context
    no_deps: bool


def _find_task_by_args(comp: Component, args: argparse.Namespace) -> Task:
    task_name = args.task
    try:
        task = comp.tasks()[task_name]
    except KeyError:
        raise ReadRunParamsError("\n".join([f"Unknown task '{task_name}'.", _available_tasks_text(comp)]))

    return task


def _make_context_from_args(args: argparse.Namespace) -> Context:
    target_dir = Path(args.target_dir).resolve()

    if args.device:
        device = SshDevice(args.device)
    else:
        device = None

    if args.log_level is not None:
        log_level = LogLevel(args.log_level)
    else:
        log_level = LogLevel.INFO if args.no_capture else LogLevel.WARNING

    return Context(
        target_dir,
        device=device,
        log_level=log_level,
        update=args.update,
        local=args.local,
        hide_artifacts=args.hide_artifacts,
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
        level = params.context.log_level.level()
        for mod in modules:
            logging.getLogger(mod).setLevel(level)


def run_with_params(params: RunParams) -> None:
    _prepare_for_run(params)
    Runner(params.task, no_deps=params.no_deps).run(params.context)
