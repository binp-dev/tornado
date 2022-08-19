from __future__ import annotations
from typing import Dict

from pathlib import Path

from ferrite.components.base import Component, ComponentGroup
from ferrite.components.core import CoreTest
from ferrite.components.toolchain import CrossToolchain, HostToolchain, Target
from ferrite.components.codegen import CodegenExample
from ferrite.components.fakedev import Fakedev
from ferrite.components.all_ import AllCross, AllHost
from ferrite.components.platforms.gnu import ArmAppToolchain, Aarch64AppToolchain
from ferrite.components.rust import HostRustup, Rustup, Cargo
from ferrite.components.app import AppBase
from ferrite.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from ferrite.components.epics.app_ioc import AppIocExample


class _HostComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
    ) -> None:
        self.toolchain = HostToolchain()
        self.epics_base = EpicsBaseHost(target_dir, self.toolchain)
        self.rustup = HostRustup(target_dir)
        self.core_test = CoreTest(source_dir, target_dir, self.toolchain)
        self.codegen = CodegenExample(source_dir, target_dir, self.toolchain)
        self.app = AppBase(source_dir / "app", target_dir / "app", self.rustup)
        self.ioc_example = AppIocExample([source_dir / "ioc"], target_dir / "ioc", self.epics_base, self.app)
        self.fakedev = Fakedev(self.ioc_example)
        self.all = AllHost(self.epics_base, self.codegen, self.app, self.ioc_example, self.fakedev)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


class _CrossComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: CrossToolchain,
        rustup: Rustup,
    ) -> None:
        self.toolchain = toolchain
        self.rustup = rustup
        self.app = AppBase(source_dir / "app", target_dir / "app", self.rustup)
        self.all = AllCross(self.app)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


class _Components(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
    ):
        self.host = _HostComponents(source_dir, target_dir)

        self.arm = _CrossComponents(
            source_dir,
            target_dir,
            ArmAppToolchain("arm", target_dir),
            Rustup("arm", Target.from_str("armv7-unknown-linux-gnueabihf"), target_dir),
        )

        self.aarch64 = _CrossComponents(
            source_dir,
            target_dir,
            Aarch64AppToolchain("aarch64", target_dir),
            Rustup("aarch64", Target.from_str("aarch64-unknown-linux-gnu"), target_dir),
        )

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


def make_components(base_dir: Path, target_dir: Path) -> ComponentGroup:
    source_dir = base_dir / "source"
    assert source_dir.exists()

    tree = _Components(source_dir, target_dir)
    tree._update_names()

    return tree
