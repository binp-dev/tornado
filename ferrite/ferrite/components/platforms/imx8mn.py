from __future__ import annotations

from pathlib import Path, PurePosixPath

from ferrite.components.freertos import Freertos
from ferrite.components.platforms.base import AppPlatform, McuPlatform, Platform
from ferrite.components.toolchain import Target, CrossToolchain
from ferrite.components.mcu import McuDeployer
from ferrite.remote.base import Device


class Imx8mnAppToolchain(CrossToolchain):

    def __init__(self, target_dir: Path) -> None:
        super().__init__(
            name="imx8mn",
            target=Target("aarch64", "linux", "gnu"),
            target_dir=target_dir,
            dir_name="gcc-linaro-11.2.1-2021.12-x86_64_aarch64-linux-gnu",
            archive="gcc-linaro-11.2.1-2021.12-x86_64_aarch64-linux-gnu.tar.xz",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-linaro-11.2.1-2021.12-x86_64_aarch64-linux-gnu.tar.xz",
                "https://snapshots.linaro.org/gnu-toolchain/11.2-2021.12-1/aarch64-linux-gnu/gcc-linaro-11.2.1-2021.12-x86_64_aarch64-linux-gnu.tar.xz",
            ],
        )


class Imx8mnMcuToolchain(CrossToolchain):

    def __init__(self, target_dir: Path) -> None:
        super().__init__(
            name="imx8mn",
            target=Target("arm", "none", "eabi"),
            target_dir=target_dir,
            dir_name="gcc-arm-none-eabi-9-2020-q2-update",
            archive="gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2",
                "https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2",
            ],
        )


class Imx8mnFreertos(Freertos):

    def __init__(self, target_dir: Path) -> None:
        branch = "mcuxpresso_sdk_2.9.x-var01"
        super().__init__(target_dir / branch, branch)


class Imx8mnMcuDeployer(McuDeployer):

    def deploy(self, build_dir: Path, device: Device) -> None:
        device.store(
            build_dir / "m7image.bin",
            PurePosixPath("/boot/m7image.bin"),
        )


class Imx8mnPlatform(Platform):

    def __init__(self, target_dir: Path) -> None:
        super().__init__(
            McuPlatform(Imx8mnMcuToolchain(target_dir), Imx8mnFreertos(target_dir), Imx8mnMcuDeployer()),
            AppPlatform(Imx8mnAppToolchain(target_dir)),
        )
