from __future__ import annotations
from typing import Dict

import argparse
import logging
from dataclasses import dataclass
from colorama import init as colorama_init, Fore, Style

from ferrite.components.base import Context, Task
from ferrite.remote.ssh import SshDevice
from ferrite.manage.tree import ComponentsDict


def _comp_list_text(comps: ComponentsDict) -> str:
    return "\n".join([
        "Available components:",
        *[f"\t{name}" for name in comps.keys()],
    ])


def add_parser_args(parser: argparse.ArgumentParser, comps: ComponentsDict) -> None:
    parser.formatter_class = argparse.RawTextHelpFormatter

    parser.add_argument(
        "comptask",
        type=str,
        metavar="<component>.<task>",
        help="\n".join([
            "Component and task you want to run.",
            _comp_list_text(comps),
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


class ReadRunParamsError(RuntimeError):
    pass


@dataclass
class RunParams:
    task: Task
    context: Context
    no_deps: bool = False


def _find_task_by_args(comps: ComponentsDict, args: argparse.Namespace) -> Task:
    names = args.comptask.rsplit(".", 1)
    comp_name = names[0]
    try:
        comp = comps[comp_name]
    except KeyError:
        raise ReadRunParamsError("\n".join([f"Unknown component '{comp_name}'.", _comp_list_text(comps)]))

    tasks_list_text = "\n".join([
        f"Available tasks for component '{comp_name}':",
        *[f"\t{name}" for name in comp.tasks().keys()],
    ])
    if len(names) == 1:
        raise ReadRunParamsError("\n".join([
            "No task provided.",
            tasks_list_text,
        ]))
    elif len(names) == 2:
        task_name = names[1]
        try:
            task = comp.tasks()[task_name]
        except KeyError:
            raise ReadRunParamsError("\n".join([
                f"Unknown task '{task_name}'.",
                tasks_list_text,
            ]))
    else:
        raise ReadRunParamsError("Bad action syntax. Expected format: '<component>.<task>'.")

    return task


def _make_context_from_args(args: argparse.Namespace) -> Context:
    device = None
    if args.device:
        device = SshDevice(args.device)

    capture = not args.no_capture

    return Context(
        device=device,
        capture=capture,
    )


def read_run_params(args: argparse.Namespace, comps: ComponentsDict) -> RunParams:
    try:
        task = _find_task_by_args(comps, args)
    except ReadRunParamsError as e:
        print(e)
        exit(1)

    context = _make_context_from_args(args)

    return RunParams(task, context, no_deps=args.no_deps)


def _prepare_for_run(params: RunParams) -> None:
    colorama_init()

    if params.context.capture:
        log_level = logging.WARNING
    else:
        log_level = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s] %(message)s', level=log_level, force=True)


def _print_title(text: str, style: Fore = None, end: bool = True) -> None:
    if style is not None:
        text = style + text + Style.RESET_ALL
    print(text, flush=True, end=("" if not end else None))


def _run_task(context: Context, task: Task, complete_tasks: Dict[str, Task], no_deps: bool = False) -> None:
    if task.name() in complete_tasks:
        return

    if not no_deps:
        for dep in task.dependencies():
            _run_task(context, dep, complete_tasks, no_deps=no_deps)

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

    complete_tasks[task.name()] = task


def run_with_params(params: RunParams) -> None:
    _prepare_for_run(params)
    _run_task(params.context, params.task, {}, no_deps=params.no_deps)
