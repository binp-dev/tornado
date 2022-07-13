from __future__ import annotations

from dataclasses import dataclass

from ferrite.components.toolchain import CrossToolchain
from ferrite.components.freertos import Freertos
from ferrite.components.mcu import McuDeployer
from ferrite.components.rust import Rustup


@dataclass
class McuPlatform:
    toolchain: CrossToolchain
    freertos: Freertos
    deployer: McuDeployer


@dataclass
class AppPlatform:
    toolchain: CrossToolchain
    rustup: Rustup


@dataclass
class Platform:
    mcu: McuPlatform
    app: AppPlatform
