from __future__ import annotations
from typing import Dict

from pathlib import Path

from ferrite.components.base import Component, ComponentGroup
from ferrite.components.toolchain import HostToolchain
from ferrite.components.codegen import CodegenTest
from ferrite.components.app import AppTest
from ferrite.components.all_ import All


class FerriteComponents(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
    ) -> None:
        toolchain = HostToolchain()
        self.codegen = CodegenTest(source_dir, target_dir, toolchain)
        self.app_test = AppTest(source_dir, target_dir, toolchain)
        self.all = All(self.codegen, self.app_test)

    def components(self) -> Dict[str, Component | ComponentGroup]:
        return self.__dict__


def make_components(base_dir: Path, target_dir: Path) -> FerriteComponents:
    source_dir = base_dir / "source"
    assert source_dir.exists()

    tree = FerriteComponents(source_dir, target_dir)
    tree._update_names()

    return tree
