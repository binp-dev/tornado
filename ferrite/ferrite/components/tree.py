from __future__ import annotations
from typing import Dict

from pathlib import Path

from ferrite.components.base import Component, DictComponent, ComponentGroup, TaskList
from ferrite.components.platforms.host import HostAppPlatform
from ferrite.components.protogen import ProtogenTest
from ferrite.components.rust import Cargo

from ferrite.protogen.test import make_test_generator


class Components(ComponentGroup):

    def __init__(
        self,
        source_dir: Path,
        target_dir: Path,
        platform: HostAppPlatform,
    ) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.protogen = ProtogenTest("protogen", source_dir, target_dir / "protogen", make_test_generator(), False, self.rustc)
        self.app = Cargo(source_dir / "app", target_dir / "app", self.rustc)
        self.all = DictComponent({"test": TaskList([self.protogen.test_task, self.app.test_task])})

    def components(self) -> Dict[str, Component]:
        return self.__dict__


def make_components(base_dir: Path, target_dir: Path) -> ComponentGroup:
    tree = Components(base_dir / "source", target_dir, HostAppPlatform(target_dir))
    tree._update_names()
    return tree
