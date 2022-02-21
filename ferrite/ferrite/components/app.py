from __future__ import annotations
from typing import Dict, List

from pathlib import Path
from dataclasses import dataclass
from copy import copy

from ferrite.components.base import Task
from ferrite.components.cmake import Cmake, CmakeRunnable
from ferrite.components.conan import CmakeWithConan
from ferrite.components.toolchain import HostToolchain, CrossToolchain, Toolchain


class AppBase(Cmake):

    @property
    def lib_src_dir(self) -> Path:
        return self.src_dir


class AppBaseHost(AppBase, CmakeWithConan):

    def __init__(
        self,
        src_dir: Path,
        build_dir: Path,
        toolchain: HostToolchain,
        target: str,
        opts: List[str] = [],
        envs: Dict[str, str] = {},
        deps: List[Task] = [],
    ):
        super().__init__(
            src_dir,
            build_dir,
            toolchain,
            target,
            opts=copy(opts),
            envs=copy(envs),
            deps=copy(deps),
        )


@dataclass
class AppBaseCross(AppBase):

    def __init__(
        self,
        src_dir: Path,
        build_dir: Path,
        toolchain: CrossToolchain,
        target: str,
        cmake_toolchain_path: Path,
        opts: List[str] = [],
        envs: Dict[str, str] = {},
        deps: List[Task] = [],
    ):
        super().__init__(
            src_dir,
            build_dir,
            toolchain,
            target,
            opts=[
                *opts,
                f"-DCMAKE_TOOLCHAIN_FILE={cmake_toolchain_path}",
            ],
            envs={
                **envs,
                "TOOLCHAIN_DIR": str(toolchain.path),
                "TARGET_TRIPLE": str(toolchain.target),
            },
            deps=copy(deps),
        )
        self.cmake_toolchain_path = cmake_toolchain_path


class AppBaseTest(CmakeWithConan, CmakeRunnable):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
    ):
        super().__init__(
            source_dir / "app" / "base_test",
            target_dir / "app_test",
            toolchain,
            target="app_base_test",
            opts=["-DCMAKE_BUILD_TYPE=Debug"],
        )


class AppExample(AppBaseCross):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: CrossToolchain,
    ):
        super().__init__(
            source_dir / "app" / "example",
            target_dir / "app",
            toolchain,
            target="app_example",
            cmake_toolchain_path=(source_dir / "app" / "cmake" / "toolchain.cmake"),
            opts=["-DCMAKE_BUILD_TYPE=Debug"],
        )
