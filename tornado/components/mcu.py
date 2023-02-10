from __future__ import annotations
from typing import List

from pathlib import Path

from ferrite.utils.path import TargetPath
from ferrite.components.base import Context
from ferrite.components.compiler import GccCross
from ferrite.components.freertos import Freertos
from ferrite.components.mcu import McuBase, McuDeployer
from ferrite.components.rust import Rustc, RustcHost, RustcCross, Cargo

from tornado.components.config import Config
from tornado.components.ipp import Ipp
from tornado.info import path as self_path


class Mcu(McuBase):

    def __init__(
        self,
        gcc: GccCross,
        rustc: RustcCross,
        freertos: Freertos,
        deployer: McuDeployer,
        config: Config,
        ipp: Ipp,
    ):
        user = McuUser(rustc, TargetPath("tornado/mcu/user"))
        super().__init__(
            self_path / "source/mcu/main",
            TargetPath("tornado/mcu/main"),
            gcc,
            freertos,
            deployer,
            target="m7image.elf",
            deps=[config.generate_task, ipp.generate_task, user.build_task],
        )
        self.config = config
        self.ipp = ipp
        self.user = user

    def opt(self, ctx: Context) -> List[str]:
        return [
            *super().opt(ctx),
            f"-DUSER={ctx.target_path / self.user.build_dir / str(self.user.rustc.target) / 'release'}",
            f"-DIPP={ctx.target_path / self.ipp.output_dir}",
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
            features=["real"],
            default_features=False,
            release=True,
        )
