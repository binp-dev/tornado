from __future__ import annotations

from dataclasses import dataclass

from ferrite.components.toolchain import CrossToolchain
from ferrite.components.freertos import Freertos
from ferrite.components.mcu import McuDeployer


@dataclass
class McuPlatform:
    toolchain: CrossToolchain
    freertos: Freertos
    deployer: McuDeployer


@dataclass
class AppPlatform:
    toolchain: CrossToolchain


@dataclass
class Platform:
    mcu: McuPlatform
    app: AppPlatform
