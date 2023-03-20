from __future__ import annotations
from typing import Callable, Optional, Generator

from contextlib import contextmanager

from colorama import Fore, Style

from ferrite.components.base import Context, Task


def _print_title(text: str, style: Optional[str] = None, end: bool = True) -> None:
    if style is not None:
        text = style + text + Style.RESET_ALL
    print(text, flush=True, end=("" if not end else None))


@contextmanager
def with_info(task: Task, context: Context) -> Generator[None, None, None]:
    if context.capture:
        _print_title(f"{task.name()} ... ", end=False)
    else:
        _print_title(f"\nTask '{task.name()}' started ...", Style.BRIGHT)

    try:
        yield
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
