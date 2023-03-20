from __future__ import annotations

from vortex.tasks.compiler import Target, GccCross
from vortex.tasks.rust import RustcCross


class AppGcc(GccCross):
    def __init__(self) -> None:
        super().__init__(
            name="app",
            target=Target("aarch64", "none", "linux", "gnu"),
            dir_name="gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu",
            archive="gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz",
                "https://developer.arm.com/-/media/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz",
            ],
        )


class McuGcc(GccCross):
    def __init__(self) -> None:
        super().__init__(
            name="mcu",
            target=Target("arm", "none", "eabi"),
            dir_name="gcc-arm-none-eabi-10-2020-q4-major",
            archive="gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2",
                "https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2",
            ],
        )


class AppRustc(RustcCross):
    def __init__(self, gcc: GccCross):
        super().__init__(gcc.name, Target.from_str("aarch64-unknown-linux-gnu"), gcc)


class McuRustc(RustcCross):
    def __init__(self, gcc: GccCross):
        super().__init__(gcc.name, Target.from_str("thumbv7em-none-eabihf"), gcc)
