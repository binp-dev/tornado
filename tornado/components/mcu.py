from __future__ import annotations

from pathlib import Path

from ferrite.components.compiler import GccCross
from ferrite.components.freertos import Freertos
from ferrite.components.mcu import McuBase, McuDeployer

from tornado.components.ipp import Ipp


class Mcu(McuBase):

    def __init__(
        self,
        ferrite_dir: Path,
        source_dir: Path,
        target_dir: Path,
        gcc: GccCross,
        freertos: Freertos,
        deployer: McuDeployer,
        ipp: Ipp,
    ):
        super().__init__(
            "mcu",
            source_dir / f"mcu",
            target_dir,
            gcc,
            freertos,
            deployer,
            target="m7image.elf",
            opts=[f"-DFERRITE={ferrite_dir}", f"-DIPP={ipp.output_dir}"],
            deps=[ipp.generate_task],
        )
        self.ferrite_dir = ferrite_dir
        self.ipp = ipp
