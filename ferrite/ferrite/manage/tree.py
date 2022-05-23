from __future__ import annotations
from typing import Dict

from pathlib import Path

from ferrite.components.base import Component, ComponentGroup
from ferrite.components.core import CoreTest
from ferrite.components.toolchain import CrossToolchain, HostToolchain
from ferrite.components.codegen import CodegenExample
from ferrite.components.app import AppBaseTest, AppExample
from ferrite.components.all_ import AllCross, AllHost
from ferrite.components.platforms.gnu import ArmAppToolchain, Aarch64AppToolchain


class _HostComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
    ) -> None:
        toolchain = HostToolchain()
        self.core_test = CoreTest(source_dir, target_dir, toolchain)
        self.codegen = CodegenExample(source_dir, target_dir, toolchain)
        self.app_test = AppBaseTest(source_dir, target_dir, toolchain)
        self.all = AllHost(self.core_test, self.codegen, self.app_test)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


class _CrossComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        toolchain: CrossToolchain,
    ) -> None:
        self.toolchain = toolchain
        self.app = AppExample(source_dir, target_dir, self.toolchain)
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
        self.arm = _CrossComponents(source_dir, target_dir, ArmAppToolchain("arm", target_dir))
        self.aarch64 = _CrossComponents(source_dir, target_dir, Aarch64AppToolchain("aarch64", target_dir))

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


def make_components(base_dir: Path, target_dir: Path) -> ComponentGroup:
    source_dir = base_dir / "source"
    assert source_dir.exists()

    tree = _Components(source_dir, target_dir)
    tree._update_names()

    return tree
