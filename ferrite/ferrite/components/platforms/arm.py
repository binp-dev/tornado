from __future__ import annotations

from pathlib import Path

from ferrite.components.compiler import Target, GccCross
from ferrite.components.rust import RustcCross
from ferrite.components.platforms.base import AppPlatform


class ArmAppToolchain(GccCross):

    def __init__(self, name: str) -> None:
        super().__init__(
            name=name,
            target=Target("arm", "none", "linux", "gnueabihf"),
            dir_name="gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf",
            archive="gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz",
                "https://developer.arm.com/-/media/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz",
            ],
        )


class ArmAppRustc(RustcCross):

    def __init__(self, postfix: str, gcc: GccCross):
        super().__init__(postfix, Target.from_str("armv7-unknown-linux-gnueabihf"), gcc)


class ArmAppPlatform(AppPlatform):

    def __init__(self, name: str) -> None:
        gcc = ArmAppToolchain(name)
        super().__init__(gcc, ArmAppRustc(name, gcc))


class Aarch64AppToolchain(GccCross):

    def __init__(self, name: str) -> None:
        super().__init__(
            name=name,
            target=Target("aarch64", "none", "linux", "gnu"),
            dir_name="gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu",
            archive="gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz",
                "https://developer.arm.com/-/media/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz",
            ],
        )


class Aarch64AppRustc(RustcCross):

    def __init__(self, postfix: str, gcc: GccCross):
        super().__init__(postfix, Target.from_str("aarch64-unknown-linux-gnu"), gcc)


class Aarch64AppPlatform(AppPlatform):

    def __init__(self, name: str) -> None:
        gcc = Aarch64AppToolchain(name)
        super().__init__(gcc, Aarch64AppRustc(name, gcc))
