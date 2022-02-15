from __future__ import annotations
from typing import List, Optional

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.cmake import Cmake, CmakeRunnable
from ferrite.components.conan import CmakeWithConan
from ferrite.components.toolchain import HostToolchain, CrossToolchain, Toolchain


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

    @property
    def lib_src_dir(self) -> Path:
        return self.src_dir


class AppTest(CmakeWithConan, CmakeRunnable):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
    ):
        super().__init__(
            source_dir / "app" / "test",
            target_dir / "app_test",
            toolchain,
            target="app_base_test",
        )


class AppExample(Cmake):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
    ):
        super().__init__(
            source_dir / "app" / "example",
            target_dir / "app",
            toolchain,
            target="app_example",
        )
