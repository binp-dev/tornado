from __future__ import annotations
from typing import List

from pathlib import Path

from ferrite.utils.path import TargetPath
from ferrite.components.base import Context
from ferrite.components.compiler import GccCross
from ferrite.components.freertos import Freertos
from ferrite.components.mcu import McuBase, McuDeployer
from ferrite.info import path as ferrite_path

from tornado.components.config import Config
from tornado.components.ipp import Ipp
from tornado.info import path as self_path


class Mcu(McuBase):

    def __init__(
        self,
        gcc: GccCross,
        freertos: Freertos,
        deployer: McuDeployer,
        config: Config,
        ipp: Ipp,
    ):
        super().__init__(
            self_path / "source/mcu/main",
            TargetPath("tornado/mcu"),
            gcc,
            freertos,
            deployer,
            target="m7image.elf",
            deps=[config.generate_task, ipp.generate_task],
        )
        self.config = config
        self.ipp = ipp

    def opt(self, ctx: Context) -> List[str]:
        return [
            *super().opt(ctx),
            f"-DIPP={ctx.target_path / self.ipp.output_dir}",
        ]
