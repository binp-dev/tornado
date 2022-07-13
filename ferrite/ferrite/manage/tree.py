from __future__ import annotations
from typing import Dict

from pathlib import Path

from ferrite.components.base import Component, ComponentGroup
from ferrite.components.core import CoreTest
from ferrite.components.toolchain import CrossToolchain, HostToolchain, Target
from ferrite.components.codegen import CodegenExample
from ferrite.components.app import AppBaseTest, AppExample
from ferrite.components.all_ import AllCross, AllHost
from ferrite.components.platforms.gnu import ArmAppToolchain, Aarch64AppToolchain
from ferrite.components.rust import HostRustup, Rustup, Cargo, CargoWithTest


class _HostComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
    ) -> None:
        toolchain = HostToolchain()
        self.rustup = HostRustup(target_dir)
        self.core_test = CoreTest(source_dir, target_dir, toolchain)
        self.codegen = CodegenExample(source_dir, target_dir, toolchain)
        self.app_test = AppBaseTest(source_dir, target_dir, toolchain)
        self.rust_test = CargoWithTest(source_dir / "app" / "rust_test", target_dir / "rust_test", self.rustup)
        self.all = AllHost(self.core_test, self.codegen, self.app_test, self.rust_test)

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
        self.app = AppExample(source_dir, target_dir, self.toolchain)
        self.rust_test = Cargo(source_dir / "app" / "rust_test", target_dir / "rust_test", self.rustup)
        self.all = AllCross(self.app, self.rust_test)

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
