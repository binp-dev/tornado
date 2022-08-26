from __future__ import annotations
from typing import Dict

from pathlib import Path

from ferrite.components.base import Component, ComponentGroup
from ferrite.components.platforms.base import AppPlatform
from ferrite.components.platforms.host import HostAppPlatform
from ferrite.components.codegen import CodegenExample
from ferrite.components.fakedev import Fakedev
from ferrite.components.all_ import AllCross, AllHost
from ferrite.components.platforms.arm import Aarch64AppPlatform, ArmAppPlatform
from ferrite.components.app import AppBase
from ferrite.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from ferrite.components.epics.ioc_example import IocHostExample, IocCrossExample


class _HostComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        platform: HostAppPlatform,
    ) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.epics_base = EpicsBaseHost(target_dir, self.gcc)
        self.codegen = CodegenExample(source_dir, target_dir, self.rustc)
        self.app = AppBase(source_dir / "app", target_dir / "app", self.rustc)
        self.ioc_example = IocHostExample(source_dir, target_dir, self.epics_base, self.app)
        self.fakedev = Fakedev(self.ioc_example)
        self.all = AllHost(self.epics_base, self.codegen, self.app, self.ioc_example, self.fakedev)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


class _CrossComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        platform: AppPlatform,
        host_components: _HostComponents,
    ) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.epics_base = EpicsBaseCross(target_dir, self.gcc, host_components.epics_base)
        self.app = AppBase(source_dir / "app", target_dir / "app", self.rustc)
        self.ioc_example = IocCrossExample(source_dir, target_dir, self.epics_base, self.app)
        self.all = AllCross(self.epics_base, self.app, self.ioc_example)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


class _Components(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
    ):
        self.host = _HostComponents(
            source_dir,
            target_dir,
            HostAppPlatform(target_dir),
        )

        self.arm = _CrossComponents(
            source_dir,
            target_dir,
            ArmAppPlatform("arm", target_dir),
            self.host,
        )

        self.aarch64 = _CrossComponents(
            source_dir,
            target_dir,
            Aarch64AppPlatform("aarch64", target_dir),
            self.host,
        )

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


def make_components(base_dir: Path, target_dir: Path) -> ComponentGroup:
    source_dir = base_dir / "source"
    assert source_dir.exists()

    tree = _Components(source_dir, target_dir)
    tree._update_names()

    return tree
