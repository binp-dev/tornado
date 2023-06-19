from __future__ import annotations

from pathlib import Path

from vortex.utils.path import TargetPath
from vortex.tasks.compiler import Gcc
from vortex.tasks.cmake import Cmake


class Plugin(Cmake):
    def __init__(self, src_dir: Path, build_dir: TargetPath, cc: Gcc):
        super().__init__(src_dir, build_dir, cc)
        self.lib_dir = self.build_dir
        self.lib_name = "libapp-plugin.so"
