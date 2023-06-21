from __future__ import annotations
from typing import List

from pathlib import Path

from vortex.utils.path import TargetPath
from vortex.tasks.base import Context, task
from vortex.tasks.binary import DynamicLib
from vortex.tasks.cmake import Cmake
from vortex.tasks.epics.epics_base import AbstractEpicsBase


class Plugin(Cmake, DynamicLib):
    def __init__(self, src_dir: Path, build_dir: TargetPath, epics_base: AbstractEpicsBase):
        super().__init__(src_dir, build_dir, epics_base.cc)
        DynamicLib.__init__(self, self.build_dir, "app-plugin")
        self.epics_base = epics_base

    def opt(self, ctx: Context) -> List[str]:
        return [f"-DEPICS_BASE={ctx.target_path / self.epics_base.install_dir}"]

    @task
    def build(self, ctx: Context) -> None:
        super().build(ctx, verbose=True)
