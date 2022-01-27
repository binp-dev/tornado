from __future__ import annotations
from typing import Dict

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Component, ComponentGroup
from ferrite.components.toolchain import HostToolchain, CrossToolchain
from ferrite.components.freertos import Freertos
from ferrite.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from ferrite.components.mcu import Mcu
from ferrite.components.codegen import CodegenTest
from ferrite.components.ipp import Ipp
from ferrite.components.app import App, AppTest
from ferrite.components.epics.ioc import AppIoc
from ferrite.components.all_ import AllHost, AllCross
from ferrite.components.platforms.base import Platform
from ferrite.components.platforms.imx7 import Imx7Platform
from ferrite.components.platforms.imx8mn import Imx8mnPlatform

import ferrite.components.toolchain as toolchain


class FerriteHostComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
    ) -> None:
        self.toolchain = toolchain
        self.epics_base = EpicsBaseHost(target_dir, toolchain)
        self.codegen = CodegenTest(source_dir, target_dir, toolchain)
        self.ipp = Ipp(source_dir, target_dir, toolchain)
        self.app_test = AppTest(source_dir, target_dir, toolchain)
        self.app = App(source_dir, target_dir, toolchain, self.ipp)
        self.ioc = AppIoc(source_dir, target_dir, self.epics_base, self.app, toolchain)
        self.all = AllHost(self.epics_base, self.codegen, self.ipp, self.app_test, self.app, self.ioc)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


class FerriteCrossComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        host_components: FerriteHostComponents,
        platform: Platform,
    ) -> None:
        self.app_toolchain = platform.app.toolchain
        self.mcu_toolchain = platform.mcu.toolchain
        self.freertos = platform.mcu.freertos
        self.epics_base = EpicsBaseCross(target_dir, self.app_toolchain, host_components.epics_base)
        self.app = App(source_dir, target_dir, self.app_toolchain, host_components.ipp)
        self.ioc = AppIoc(source_dir, target_dir, self.epics_base, self.app, self.app_toolchain)
        self.mcu = Mcu(source_dir, target_dir, self.mcu_toolchain, self.freertos, host_components.ipp, platform.mcu.deployer)
        self.all = AllCross(self.epics_base, self.app, self.ioc, self.mcu)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


@dataclass
class FerriteComponents(ComponentGroup):
    host: FerriteHostComponents
    cross: Dict[str, FerriteCrossComponents]

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return {
            "host": self.host,
            **self.cross,
        }


def make_components(base_dir: Path, target_dir: Path) -> FerriteComponents:
    source_dir = base_dir / "source"
    assert source_dir.exists()

    host = FerriteHostComponents(
        source_dir,
        target_dir,
        toolchain.HostToolchain(),
    )
    tree = FerriteComponents(
        host, {
            "imx7": FerriteCrossComponents(
                source_dir,
                target_dir,
                host,
                Imx7Platform(target_dir),
            ),
            "imx8mn": FerriteCrossComponents(
                source_dir,
                target_dir,
                host,
                Imx8mnPlatform(target_dir),
            ),
        }
    )
    tree._update_names()
    return tree
