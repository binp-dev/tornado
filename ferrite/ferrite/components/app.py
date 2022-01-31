from __future__ import annotations
from typing import Dict, List, Optional

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.cmake import CmakeWithTest
from ferrite.components.conan import CmakeWithConan
from ferrite.components.ipp import Ipp
from ferrite.components.toolchain import Toolchain, HostToolchain, CrossToolchain


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


class App(AppBase):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
        ipp: Ipp,
    ):
        src_dir = source_dir / "app"
        build_dir = target_dir / f"app_{toolchain.name}"

        opts: List[str] = [f"-DIPP_GEN_DIR={ipp.gen_dir}"]
        if isinstance(toolchain, HostToolchain):
            target = "app_fakedev"
            opts.append("-DAPP_FAKEDEV=1")
        if isinstance(toolchain, CrossToolchain):
            target = "app"
            opts.append("-DAPP_MAIN=1")

        super().__init__(
            src_dir,
            build_dir,
            toolchain,
            opts=opts,
            deps=[ipp.generate_task],
            target=target,
            cmake_toolchain_path=src_dir / "armgcc.cmake",
            disable_conan=isinstance(toolchain, CrossToolchain),
        )
        self.ipp = ipp


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
