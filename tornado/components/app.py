from __future__ import annotations
from typing import Dict, List

from pathlib import Path

from ferrite.components.app import AppBase
from ferrite.components.toolchain import Toolchain, HostToolchain, CrossToolchain

from tornado.components.ipp import Ipp


class App(AppBase):

    def __init__(
        self,
        source_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
        ipp: Ipp,
    ):
        src_dir = source_dir / "app"
        lib_src_dir = ferrite_source_dir / "app"
        build_dir = target_dir / f"app_{toolchain.name}"

        opts: List[str] = [
            f"-DAPP_LIB_DIR={lib_src_dir}",
            f"-DIPP_GEN_DIR={ipp.gen_dir}",
        ]
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
            cmake_toolchain_path=(lib_src_dir / "armgcc.cmake"),
            disable_conan=isinstance(toolchain, CrossToolchain),
        )
        self.lib_src_dir = lib_src_dir
        self.ipp = ipp
