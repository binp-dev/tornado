from __future__ import annotations

from pathlib import Path

from ferrite.components.toolchain import Target, CrossToolchain


class ArmAppToolchain(CrossToolchain):

    def __init__(self, name: str, target_dir: Path) -> None:
        super().__init__(
            name=name,
            target=Target("arm", "none", "linux", "gnueabihf"),
            target_dir=target_dir,
            dir_name="gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf",
            archive="gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz",
                "https://developer.arm.com/-/media/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz",
            ],
        )


class Aarch64AppToolchain(CrossToolchain):

    def __init__(self, name: str, target_dir: Path) -> None:
        super().__init__(
            name=name,
            target=Target("aarch64", "none", "linux", "gnu"),
            target_dir=target_dir,
            dir_name="gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu",
            archive="gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz",
                "https://developer.arm.com/-/media/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz",
            ],
        )
