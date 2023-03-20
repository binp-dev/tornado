from __future__ import annotations
from typing import Dict

from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Component, ComponentGroup
from ferrite.components.toolchain import HostToolchain
from ferrite.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from ferrite.components.platforms.base import Platform
from ferrite.components.platforms.imx8mn import Imx8mnPlatform

from tornado.components.ipp import Ipp
from tornado.components.app import AppReal, AppFake
from tornado.components.epics.app_ioc import AppIocHost, AppIocCross
from tornado.components.mcu import Mcu
from tornado.components.all_ import AllHost, AllCross


class _HostComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        toolchain: HostToolchain,
    ) -> None:
        self.toolchain = toolchain
        self.epics_base = EpicsBaseHost(target_dir, toolchain)
        self.ipp = Ipp(source_dir, ferrite_source_dir, target_dir, toolchain)
        self.app = AppFake(source_dir, ferrite_source_dir, target_dir, toolchain, self.ipp)
        self.ioc_fakedev = AppIocHost(
            source_dir,
            ferrite_source_dir,
            target_dir,
            self.epics_base,
            self.app,
        )
        self.all = AllHost(self.epics_base, self.ipp, self.app, self.ioc_fakedev)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


class _CrossComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        host_components: _HostComponents,
        platform: Platform,
    ) -> None:
        self.app_toolchain = platform.app.toolchain
        self.mcu_toolchain = platform.mcu.toolchain
        self.freertos = platform.mcu.freertos
        self.epics_base = EpicsBaseCross(
            target_dir,
            self.app_toolchain,
            host_components.epics_base,
        )
        self.app = AppReal(
            source_dir,
            ferrite_source_dir,
            target_dir,
            self.app_toolchain,
            host_components.ipp,
        )
        self.ioc = AppIocCross(
            source_dir,
            ferrite_source_dir,
            target_dir,
            self.epics_base,
            self.app,
        )
        self.mcu = Mcu(
            source_dir,
            ferrite_source_dir,
            target_dir,
            self.mcu_toolchain,
            self.freertos,
            platform.mcu.deployer,
            host_components.ipp,
        )
        self.all = AllCross(self.epics_base, self.app, self.ioc, self.mcu)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


@dataclass
class _Components(ComponentGroup):
    host: _HostComponents
    cross: Dict[str, _CrossComponents]

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return {
            "host": self.host,
            **self.cross,
        }


def make_components(base_dir: Path, ferrite_dir: Path, target_dir: Path) -> ComponentGroup:
    source_dir = base_dir / "source"
    assert source_dir.exists()

    ferrite_source_dir = ferrite_dir / "source"
    assert ferrite_source_dir.exists()

    host = _HostComponents(
        source_dir,
        ferrite_source_dir,
        target_dir,
        HostToolchain(),
    )
    device = _CrossComponents(
        source_dir,
        ferrite_source_dir,
        target_dir,
        host,
        Imx8mnPlatform(target_dir),
    )
    tree = _Components(host, {"device": device})
    tree._update_names()
    return tree
