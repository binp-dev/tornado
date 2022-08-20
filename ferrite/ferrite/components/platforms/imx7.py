from __future__ import annotations

from pathlib import Path, PurePosixPath

from ferrite.components.compiler import Target, GccCross
from ferrite.components.mcu import McuDeployer
from ferrite.components.freertos import Freertos
from ferrite.components.platforms.base import McuPlatform, Platform
from ferrite.components.platforms.arm import ArmAppPlatform
from ferrite.remote.base import Device


class Imx7AppPlatform(ArmAppPlatform):

    def __init__(self, target_dir: Path) -> None:
        super().__init__("imx7", target_dir)


class Imx7McuToolchain(GccCross):

    def __init__(self, target_dir: Path) -> None:
        super().__init__(
            name="imx7",
            target=Target("arm", "none", "eabi"),
            target_dir=target_dir,
            dir_name="gcc-arm-none-eabi-5_4-2016q3",
            archive="gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2",
                "https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2",
            ],
        )


class Imx7McuDeployer(McuDeployer):

    def deploy(self, build_dir: Path, device: Device) -> None:
        device.store(
            build_dir / "release/m4image.bin",
            PurePosixPath("/m4image.bin"),
        )
        device.run(["bash", "-c", " && ".join([
            "mount /dev/mmcblk2p1 /mnt",
            "mv /m4image.bin /mnt",
            "umount /mnt",
        ])])


class Imx7Freertos(Freertos):

    def __init__(self, target_dir: Path) -> None:
        branch = "freertos_bsp_1.0.1_imx7d-var01"
        super().__init__(target_dir / branch, branch)


class Imx7Platform(Platform):

    def __init__(self, target_dir: Path) -> None:
        super().__init__(
            McuPlatform(Imx7McuToolchain(target_dir), Imx7Freertos(target_dir), Imx7McuDeployer()),
            Imx7AppPlatform(target_dir),
        )
