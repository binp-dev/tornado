from __future__ import annotations

from pathlib import Path

from vortex.utils.path import TargetPath
from vortex.tasks.compiler import Gcc
from vortex.tasks.cmake import Cmake
from vortex.tasks.binary import DynamicLib


class Plugin(Cmake, DynamicLib):
    def __init__(self, src_dir: Path, build_dir: TargetPath, cc: Gcc):
        super().__init__(src_dir, build_dir, cc)
        DynamicLib.__init__(self, self.build_dir, "app-plugin")
