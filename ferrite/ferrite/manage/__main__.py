from __future__ import annotations

import argparse
import logging
from colorama import init as colorama_init, Fore, Style

from ferrite.components.base import Context, Task
from ferrite.remote.ssh import SshDevice
from ferrite.manage.tree import components

if __name__ == "__main__":
    colorama_init()

    parser = argparse.ArgumentParser(
        description="Power supply controller software development automation tool",
        usage="python3 -m manage <component>.<task> [options...]",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    available_components_text = "\n".join([
        "Available components:",
        *[f"\t{name}" for name in components.keys()],
    ])
    parser.add_argument(
        "comptask",
        type=str,
        metavar="<component>.<task>",
        help="\n".join([
            "Component and task you want to run.",
            available_components_text,
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
    args = parser.parse_args()

    names = args.comptask.rsplit(".", 1)
    component_name = names[0]
    try:
        component = components[component_name]
    except KeyError:
        print("\n".join([f"Unknown component '{component_name}'.", available_components_text]))
        exit(1)

    available_tasks_text = "\n".join([
        f"Available tasks for component '{component_name}':",
        *[f"\t{name}" for name in component.tasks().keys()],
    ])
    if len(names) == 1:
        print("\n".join([
            "No task provided.",
            available_tasks_text,
        ]))
        exit(1)
    elif len(names) == 2:
        task_name = names[1]
        try:
            task = component.tasks()[task_name]
        except KeyError:
            print("\n".join([
                f"Unknown task '{task_name}'.",
                available_tasks_text,
            ]))
            exit(1)
    else:
        print("Bad action syntax. Expected format: '<component>.<task>'.")
        exit(1)

    device = None
    if args.device:
        device = SshDevice(args.device)

    capture = not args.no_capture
    if capture:
        log_level = logging.WARNING
    else:
        log_level = logging.DEBUG

    logging.basicConfig(format='[%(levelname)s] %(message)s', level=log_level, force=True)

    context = Context(
        device=device,
        capture=capture,
    )

    def print_title(text: str, style: Fore = None, end: bool = True) -> None:
        if style is not None:
            text = style + text + Style.RESET_ALL
        print(text, flush=True, end=("" if not end else None))

    complete_tasks = {}

    def run_task(context: Context, task: Task, no_deps: bool = False, title_length: int = 64) -> None:
        if task.name() in complete_tasks:
            return

        if not no_deps:
            for dep in task.dependencies():
                run_task(context, dep, no_deps=no_deps, title_length=title_length)

        if capture:
            print_title(f"{task.name()} ... ", end=False)
        else:
            print_title(f"\nTask '{task.name()}' started ...", Style.BRIGHT)

        try:
            task.run(context)
        except:
            if capture:
                print_title(f"FAIL", Fore.RED)
            else:
                print_title(f"Task '{task.name()}' FAILED:", Style.BRIGHT + Fore.RED)
            raise
        else:
            if capture:
                print_title(f"ok", Fore.GREEN)
            else:
                print_title(f"Task '{task.name()}' successfully completed", Style.BRIGHT + Fore.GREEN)

        complete_tasks[task.name()] = task

    run_task(context, task, no_deps=args.no_deps)
