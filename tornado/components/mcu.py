from __future__ import annotations

from pathlib import Path

from ferrite.components.toolchain import CrossToolchain
from ferrite.components.freertos import Freertos
from ferrite.components.mcu import McuBase, McuDeployer

from tornado.components.ipp import Ipp


class Mcu(McuBase):

    def __init__(
        self,
        source_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        toolchain: CrossToolchain,
        freertos: Freertos,
        deployer: McuDeployer,
        ipp: Ipp,
    ):
        super().__init__(
            "mcu",
            source_dir / f"mcu",
            target_dir,
            toolchain,
            freertos,
            deployer,
            target="m7image.elf",
            opts=[f"-DFERRITE={ferrite_source_dir}", f"-DIPP={ipp.gen_dir}"],
            deps=[ipp.generate_task],
        )
        self.ferrite_source_dir = ferrite_source_dir
        self.ipp = ipp
