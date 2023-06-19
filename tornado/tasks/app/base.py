from __future__ import annotations
from typing import Protocol, Callable

from vortex.tasks.base import Context
from vortex.utils.path import TargetPath


class Linkable(Protocol):
    lib_name: str
    lib_dir: TargetPath

    build: Callable[[Context], None]
