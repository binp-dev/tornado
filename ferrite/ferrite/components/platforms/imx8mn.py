from __future__ import annotations

from pathlib import Path, PurePosixPath

from ferrite.utils.path import TargetPath
from ferrite.components.freertos import Freertos
from ferrite.components.platforms.base import McuPlatform, Platform
from ferrite.components.compiler import Target, GccCross
from ferrite.components.mcu import McuDeployer
from ferrite.components.platforms.arm import Aarch64AppPlatform
from ferrite.remote.base import Device


class Imx8mnAppPlatform(Aarch64AppPlatform):

    def __init__(self) -> None:
        super().__init__("imx8mn")


class Imx8mnMcuToolchain(GccCross):

    def __init__(self) -> None:
        super().__init__(
            name="imx8mn",
            target=Target("arm", "none", "eabi"),
            dir_name="gcc-arm-none-eabi-10-2020-q4-major",
            archive="gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2",
                "https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2",
            ],
        )


class Imx8mnFreertos(Freertos):

    def __init__(self) -> None:
        branch = "mcuxpresso_sdk_2.10.x-var01"
        super().__init__(TargetPath(branch), branch)


class Imx8mnMcuDeployer(McuDeployer):

    def deploy(self, build_dir: Path, device: Device) -> None:
        device.store(
            build_dir / "m7image.bin",
            PurePosixPath("/boot/m7image.bin"),
        )


class Imx8mnPlatform(Platform):

    def __init__(self) -> None:
        super().__init__(
            McuPlatform(Imx8mnMcuToolchain(), Imx8mnFreertos(), Imx8mnMcuDeployer()),
            Imx8mnAppPlatform(),
        )
