from __future__ import annotations
from typing import Dict, List

import shutil
from pathlib import Path, PurePosixPath
from dataclasses import dataclass

from vortex.utils.path import TargetPath
from vortex.tasks.base import task, Task, Context
from vortex.tasks.compiler import GccCross
from vortex.tasks.rust import Rustc, RustcCross, Cargo
from vortex.tasks.cmake import Cmake

from tornado.tasks.freertos import Freertos
from tornado.manage.info import path as self_path


class McuBase(Cmake):
    def configure(self, ctx: Context) -> None:
        build_path = ctx.target_path / self.build_dir

        # Workaround to disable cmake caching (incremental build is broken anyway)
        if build_path.exists():
            shutil.rmtree(build_path)

        super().configure(ctx)

    def __init__(
        self,
        src_dir: Path,
        build_dir: TargetPath,
        cc: GccCross,
        freertos: Freertos,
        build_target: str,
        deps: List[Task] = [],
    ):
        super().__init__(src_dir, build_dir, cc, build_target=build_target)
        self.freertos = freertos

    def env(self, ctx: Context) -> Dict[str, str]:
        assert isinstance(self.cc, GccCross)
        return {
            **super().env(ctx),
            "FREERTOS_DIR": str(ctx.target_path / self.freertos.path),
            "ARMGCC_DIR": str(ctx.target_path / self.cc.path),
        }

    def opt(self, ctx: Context) -> List[str]:
        return [
            *super().opt(ctx),
            f"-DCMAKE_TOOLCHAIN_FILE={ctx.target_path / self.freertos.path / 'tools/cmake_toolchain_files/armgcc.cmake'}",
            "-DCMAKE_BUILD_TYPE=Release",
        ]

    @task
    def build(self, ctx: Context) -> None:
        self.freertos.clone(ctx)
        super().build(ctx)

    @task
    def deploy(self, ctx: Context) -> None:
        assert ctx.device is not None
        self.build(ctx)
        ctx.device.store(
            ctx.target_path / self.build_dir / "m7image.bin",
            PurePosixPath("/boot/m7image.bin"),
        )

    @task
    def deploy_and_reboot(self, ctx: Context) -> None:
        assert ctx.device is not None
        self.deploy(ctx)
        ctx.device.reboot()


class Mcu(McuBase):
    def __init__(
        self,
        gcc: GccCross,
        rustc: RustcCross,
        freertos: Freertos,
    ):
        super().__init__(
            self_path / "source/mcu/main",
            TargetPath("tornado/mcu/main"),
            gcc,
            freertos,
            build_target="m7image.elf",
        )
        self.user = McuUser(rustc, TargetPath("tornado/mcu/user"))

    def opt(self, ctx: Context) -> List[str]:
        return [
            *super().opt(ctx),
            f"-DUSER={ctx.target_path / self.user.bin_dir}",
        ]

    @task
    def build(self, ctx: Context) -> None:
        self.user.build(ctx)
        super().build(ctx)


class McuUser(Cargo):
    def __init__(
        self,
        rustc: Rustc,
        build_dir: TargetPath,
    ) -> None:
        super().__init__(
            self_path / "source/mcu/user",
            build_dir,
            rustc,
            features=["real", "panic"],
            default_features=False,
            release=True,
        )
