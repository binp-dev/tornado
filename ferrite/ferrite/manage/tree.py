from __future__ import annotations
from typing import Dict

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Component, ComponentGroup
from ferrite.components.toolchains import HostToolchain, CrossToolchain
from ferrite.components.freertos import Freertos, FreertosImx7, FreertosImx8mn
from ferrite.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from ferrite.components.mcu import Mcu
from ferrite.components.codegen import CodegenTest
from ferrite.components.ipp import Ipp
from ferrite.components.app import App, AppTest
from ferrite.components.epics.ioc import AppIoc
from ferrite.components.all_ import AllHost, AllCross

import ferrite.components.toolchains as toolchains


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
        host_components: FerriteHostComponents,
        source_dir: Path,
        target_dir: Path,
        app_toolchain: CrossToolchain,
        mcu_toolchain: CrossToolchain,
        freertos: Freertos,
    ) -> None:
        self.app_toolchain = app_toolchain
        self.mcu_toolchain = mcu_toolchain
        self.freertos = freertos
        self.epics_base = EpicsBaseCross(target_dir, app_toolchain, host_components.epics_base)
        self.app = App(source_dir, target_dir, app_toolchain, host_components.ipp)
        self.ioc = AppIoc(source_dir, target_dir, self.epics_base, self.app, app_toolchain)
        self.mcu = Mcu(source_dir, target_dir, mcu_toolchain, freertos, host_components.ipp)
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


def make_components(source_dir: Path, target_dir: Path) -> FerriteComponents:
    host = FerriteHostComponents(
        source_dir,
        target_dir,
        toolchains.HostToolchain(),
    )
    tree = FerriteComponents(
        host, {
            "imx7": FerriteCrossComponents(
                host,
                source_dir,
                target_dir,
                toolchains.AppToolchainImx7(target_dir),
                toolchains.McuToolchainImx7(target_dir),
                FreertosImx7(target_dir),
            ),
            "imx8mn": FerriteCrossComponents(
                host,
                source_dir,
                target_dir,
                toolchains.AppToolchainImx8mn(target_dir),
                toolchains.McuToolchainImx8mn(target_dir),
                FreertosImx8mn(target_dir),
            ),
        }
    )
    tree._update_names()
    return tree
