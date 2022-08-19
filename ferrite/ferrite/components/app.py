from __future__ import annotations
from typing import Dict, List

from pathlib import Path

from ferrite.components.rust import Cargo, Rustup


class AppBase(Cargo):

    def __init__(self, src_dir: Path, build_dir: Path, toolchain: Rustup):
        super().__init__(src_dir, build_dir, toolchain)
