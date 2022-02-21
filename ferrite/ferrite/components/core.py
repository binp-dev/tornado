from __future__ import annotations

from pathlib import Path

from ferrite.components.cmake import CmakeRunnable
from ferrite.components.conan import CmakeWithConan
from ferrite.components.toolchain import Toolchain


class CoreTest(CmakeWithConan, CmakeRunnable):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
    ):
        super().__init__(
            source_dir / "core" / "test",
            target_dir / "core_test",
            toolchain,
            target="core_test",
        )
