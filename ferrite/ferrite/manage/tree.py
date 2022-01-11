from __future__ import annotations
from typing import Dict

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
        toolchain: HostToolchain,
    ) -> None:
        self.toolchain = toolchain
        self.epics_base = EpicsBaseHost(toolchain)
        self.codegen = Codegen(toolchain)
        self.ipp = Ipp(toolchain, self.codegen)
        self.app = App(toolchain, self.ipp)
        self.ioc = AppIoc(self.epics_base, self.app, toolchain)
        self.all = AllHost(self.epics_base, self.codegen, self.ipp, self.app, self.ioc)


class CrossComponentStorage(ComponentStorage):

    def __init__(
        self,
        host_components: HostComponentStorage,
        app_toolchain: CrossToolchain,
        mcu_toolchain: CrossToolchain,
        freertos: Freertos,
    ) -> None:
        self.app_toolchain = app_toolchain
        self.mcu_toolchain = mcu_toolchain
        self.freertos = freertos
        self.epics_base = EpicsBaseCross(app_toolchain, host_components.epics_base)
        self.ipp = host_components.ipp
        self.app = App(app_toolchain, self.ipp)
        self.ioc = AppIoc(self.epics_base, self.app, app_toolchain)
        self.mcu = Mcu(freertos, mcu_toolchain, self.ipp)
        self.all = AllCross(self.epics_base, self.app, self.ioc, self.mcu)


host = HostComponentStorage(toolchains.HostToolchain(),)
imx7 = CrossComponentStorage(
    host,
    toolchains.AppToolchainImx7(),
    toolchains.McuToolchainImx7(),
    FreertosImx7(),
)
imx8mn = CrossComponentStorage(
    host,
    toolchains.AppToolchainImx8mn(),
    toolchains.McuToolchainImx8mn(),
    FreertosImx8mn(),
)

ComponentsDict = Dict[str, Component]

COMPONENTS: ComponentsDict = {
    **{f"host_{k}": c for k, c in host.__dict__.items()},
    **{f"imx7_{k}": c for k, c in imx7.__dict__.items()},
    **{f"imx8mn_{k}": c for k, c in imx8mn.__dict__.items()},
}

for cname, comp in COMPONENTS.items():
    for tname, task in comp.tasks().items():
        if not task._name:
            task._name = f"{cname}.{tname}"
