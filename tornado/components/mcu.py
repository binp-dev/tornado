from __future__ import annotations
from typing import List

from ferrite.utils.path import TargetPath
from ferrite.components.base import Context
from ferrite.components.compiler import GccCross
from ferrite.components.freertos import Freertos
from ferrite.components.mcu import McuBase, McuDeployer
from ferrite.components.rust import Rustc, RustcHost, RustcCross, Cargo

from tornado.info import path as self_path


class Mcu(McuBase):

    def __init__(
        self,
        gcc: GccCross,
        rustc: RustcCross,
        freertos: Freertos,
        deployer: McuDeployer,
    ):
        user = McuUser(rustc, TargetPath("tornado/mcu/user"))
        super().__init__(
            self_path / "source/mcu/main",
            TargetPath("tornado/mcu/main"),
            gcc,
            freertos,
            deployer,
            target="m7image.elf",
            deps=[user.build_task],
        )
        self.user = user

    def opt(self, ctx: Context) -> List[str]:
        return [
            *super().opt(ctx),
            f"-DUSER={ctx.target_path / self.user.bin_dir}",
        ]


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
