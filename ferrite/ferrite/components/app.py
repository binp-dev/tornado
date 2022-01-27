from __future__ import annotations
from typing import Dict, List

from pathlib import Path

from ferrite.components.cmake import CmakeWithTest
from ferrite.components.conan import CmakeWithConan
from ferrite.components.ipp import Ipp
from ferrite.components.toolchain import Toolchain, HostToolchain, CrossToolchain


class App(CmakeWithConan):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: Toolchain,
        ipp: Ipp,
    ):
        src_dir = source_dir / "app"
        build_dir = target_dir / f"app_{toolchain.name}"

        opts: List[str] = [
            "-DCMAKE_BUILD_TYPE=Debug",
            f"-DIPP_GEN_DIR={ipp.gen_dir}",
        ]
        envs: Dict[str, str] = {}
        if isinstance(toolchain, HostToolchain):
            target = "app_fakedev"
            opts.append("-DAPP_FAKEDEV=1")
        if isinstance(toolchain, CrossToolchain):
            target = "app"
            opts.append("-DAPP_MAIN=1")
            toolchain_cmake_path = src_dir / "armgcc.cmake"
            opts.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_cmake_path}")
            envs.update({
                "TOOLCHAIN_DIR": str(toolchain.path),
                "TARGET_TRIPLE": str(toolchain.target),
            })

        super().__init__(
            src_dir,
            build_dir,
            toolchain,
            opts=opts,
            envs=envs,
            deps=[ipp.generate_task],
            target=target,
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
