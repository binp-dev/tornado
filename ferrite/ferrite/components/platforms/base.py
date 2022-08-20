from __future__ import annotations

from dataclasses import dataclass

from ferrite.components.compiler import GccCross
from ferrite.components.freertos import Freertos
from ferrite.components.mcu import McuDeployer
from ferrite.components.rust import RustcCross


@dataclass
class McuPlatform:
    cc: GccCross
    freertos: Freertos
    deployer: McuDeployer


@dataclass
class AppPlatform:
    gcc: GccCross
    rustc: RustcCross


@dataclass
class Platform:
    mcu: McuPlatform
    app: AppPlatform
