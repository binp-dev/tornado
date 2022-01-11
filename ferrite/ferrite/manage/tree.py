from __future__ import annotations
from typing import Dict

from pathlib import Path

from ferrite.components.base import Component
from ferrite.components.toolchains import HostToolchain, CrossToolchain
from ferrite.components.freertos import Freertos, FreertosImx7, FreertosImx8mn
from ferrite.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from ferrite.components.mcu import Mcu
from ferrite.components.codegen import Codegen
from ferrite.components.ipp import Ipp
from ferrite.components.app import App
from ferrite.components.epics.ioc import AppIoc
from ferrite.components.all_ import AllHost, AllCross

import ferrite.components.toolchains as toolchains


class ComponentStorage:
    pass


class HostComponentStorage(ComponentStorage):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
    ) -> None:
        self.toolchain = toolchain
        self.epics_base = EpicsBaseHost(toolchain, target_dir)
        self.codegen = Codegen(source_dir, target_dir, toolchain)
        self.ipp = Ipp(source_dir, target_dir, toolchain, self.codegen)
        self.app = App(source_dir, target_dir, toolchain, self.ipp)
        self.ioc = AppIoc(source_dir, target_dir, self.epics_base, self.app, toolchain)
        self.all = AllHost(self.epics_base, self.codegen, self.ipp, self.app, self.ioc)


class CrossComponentStorage(ComponentStorage):

    def __init__(
        self,
        host_components: HostComponentStorage,
        source_dir: Path,
        target_dir: Path,
        app_toolchain: CrossToolchain,
        mcu_toolchain: CrossToolchain,
        freertos: Freertos,
    ) -> None:
        self.app_toolchain = app_toolchain
        self.mcu_toolchain = mcu_toolchain
        self.freertos = freertos
        self.epics_base = EpicsBaseCross(app_toolchain, target_dir, host_components.epics_base)
        self.ipp = host_components.ipp
        self.app = App(source_dir, target_dir, app_toolchain, self.ipp)
        self.ioc = AppIoc(source_dir, target_dir, self.epics_base, self.app, app_toolchain)
        self.mcu = Mcu(source_dir, target_dir, mcu_toolchain, freertos, self.ipp)
        self.all = AllCross(self.epics_base, self.app, self.ioc, self.mcu)


ComponentsDict = Dict[str, Component]


def make_components(source_dir: Path, target_dir: Path) -> ComponentsDict:
    host = HostComponentStorage(
        source_dir,
        target_dir,
        toolchains.HostToolchain(),
    )
    imx7 = CrossComponentStorage(
        host,
        source_dir,
        target_dir,
        toolchains.AppToolchainImx7(target_dir),
        toolchains.McuToolchainImx7(target_dir),
        FreertosImx7(target_dir),
    )
    imx8mn = CrossComponentStorage(
        host,
        source_dir,
        target_dir,
        toolchains.AppToolchainImx8mn(target_dir),
        toolchains.McuToolchainImx8mn(target_dir),
        FreertosImx8mn(target_dir),
    )

    comps: ComponentsDict = {
        **{f"host_{k}": c for k, c in host.__dict__.items()},
        **{f"imx7_{k}": c for k, c in imx7.__dict__.items()},
        **{f"imx8mn_{k}": c for k, c in imx8mn.__dict__.items()},
    }

    for cname, comp in comps.items():
        for tname, task in comp.tasks().items():
            if not task._name:
                task._name = f"{cname}.{tname}"

    return comps
