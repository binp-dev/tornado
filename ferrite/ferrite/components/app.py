from __future__ import annotations
from typing import List, Optional

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.cmake import CmakeWithTest
from ferrite.components.conan import CmakeWithConan
from ferrite.components.toolchain import HostToolchain, CrossToolchain


@dataclass
class AppBase(CmakeWithConan):
    cmake_toolchain_path: Optional[Path] = None

    def __post_init__(self) -> None:
        self.opts.append("-DCMAKE_BUILD_TYPE=Debug")

        if isinstance(self.toolchain, CrossToolchain):
            self.opts.append(f"-DCMAKE_TOOLCHAIN_FILE={self.cmake_toolchain_path}")
            self.envs.update({
                "TOOLCHAIN_DIR": str(self.toolchain.path),
                "TARGET_TRIPLE": str(self.toolchain.target),
            })

        super().__post_init__()


class AppTest(CmakeWithConan, CmakeWithTest):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
    ):
        super().__init__(
            source_dir / "app",
            target_dir / "app_test",
            toolchain,
            opts=["-DAPP_TEST=1"],
            target="app_unittest",
        )
