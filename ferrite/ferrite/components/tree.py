from __future__ import annotations
from typing import Dict

from ferrite.components.base import Component, DictComponent, ComponentGroup, TaskList
from ferrite.components.platforms.host import HostPlatform
from ferrite.components.codegen import ProtogenTest
from ferrite.components.rust import Cargo

from ferrite.utils.path import TargetPath
from ferrite.codegen.test.proto import make_test_generator
from ferrite.info import path as self_path


class Components(ComponentGroup):

    def __init__(self, platform: HostPlatform) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.codegen = ProtogenTest("codegen", TargetPath("ferrite/codegen"), make_test_generator(), False, self.rustc)
        self.app = Cargo(self_path / "source/app", TargetPath("ferrite/app"), self.rustc)
        self.all = DictComponent({"test": TaskList([self.codegen.test_task, self.app.test_task])})

    def components(self) -> Dict[str, Component]:
        return self.__dict__


def make_components() -> ComponentGroup:
    tree = Components(HostPlatform())
    tree._update_names()
    return tree
