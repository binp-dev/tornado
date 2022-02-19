from __future__ import annotations
from typing import Any, Dict, List

from pathlib import Path

from ferrite.components.app import AppBase, AppBaseHost, AppBaseCross
from ferrite.components.toolchain import Toolchain, HostToolchain, CrossToolchain

from tornado.components.ipp import Ipp


class AppCommon(AppBase):

    def __init__(
        self,
        src_dir: Path,
        ferrite_source_dir: Path,
        build_dir: Path,
        toolchain: Toolchain,
        ipp: Ipp,
        **kwargs: Any,
    ):
        super().__init__(
            src_dir,
            build_dir,
            toolchain,
            target="app",
            opts=[
                f"-DFERRITE={ferrite_source_dir}",
                f"-DIPP={ipp.gen_dir}",
            ],
            deps=[ipp.generate_task],
            **kwargs
        )
        self.ipp = ipp


class AppReal(AppCommon, AppBaseCross):

    def __init__(
        self,
        source_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        toolchain: CrossToolchain,
        ipp: Ipp,
    ):
        super().__init__(
            source_dir / "app" / "real",
            ferrite_source_dir,
            target_dir / f"app_{toolchain.name}",
            toolchain,
            ipp,
            cmake_toolchain_path=(ferrite_source_dir / "app" / "cmake" / "toolchain.cmake"),
        )


class AppFake(AppCommon, AppBaseHost):

    def __init__(
        self,
        source_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
        ipp: Ipp,
    ):
        super().__init__(
            source_dir / "app" / "fake",
            ferrite_source_dir,
            target_dir / f"app_fakedev",
            toolchain,
            ipp,
        )
