from __future__ import annotations
from typing import Dict

from pathlib import Path

from ferrite.components.base import TaskList, TaskWrapper, Component, DictComponent, ComponentGroup
from ferrite.components.platforms.base import AppPlatform
from ferrite.components.platforms.host import HostAppPlatform
from ferrite.components.platforms.arm import Aarch64AppPlatform, ArmAppPlatform
from ferrite.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross

from example.components.app import App
from example.components.frontend import FrontendHost, FrontendCross
from example.components.backend import TestBackend
from example.components.protocol import Protocol


class HostComponents(ComponentGroup):

    def __init__(
        self,
        ferrite_dir: Path,
        source_dir: Path,
        target_dir: Path,
        platform: HostAppPlatform,
    ) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.epics_base = EpicsBaseHost(target_dir, self.gcc)
        self.protocol = Protocol(ferrite_dir, target_dir, self.rustc)
        self.app = App(source_dir, target_dir, self.rustc, self.protocol)
        self.ioc = FrontendHost(ferrite_dir, source_dir, target_dir, self.epics_base, self.app)
        self.backend = TestBackend(self.ioc, self.protocol)
        self.all = DictComponent({
            "build": TaskList([self.epics_base.install_task, self.app.build_task, self.ioc.install_task]),
            "test": TaskList([self.protocol.test_task, self.app.test_task, self.backend.test_task]),
        })

    def components(self) -> Dict[str, Component]:
        return self.__dict__


class CrossComponents(ComponentGroup):

    def __init__(
        self,
        ferrite_dir: Path,
        source_dir: Path,
        target_dir: Path,
        platform: AppPlatform,
        host: HostComponents,
    ) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.epics_base = EpicsBaseCross(target_dir, self.gcc, host.epics_base)
        self.app = App(source_dir, target_dir, self.rustc, host.protocol)
        self.ioc = FrontendCross(ferrite_dir, source_dir, target_dir, self.epics_base, self.app)

        build_task = TaskList([self.epics_base.install_task, self.ioc.install_task])
        deploy_task = TaskList([self.epics_base.deploy_task, self.ioc.deploy_task])
        run_task = TaskWrapper(self.ioc.run_task, [deploy_task])
        self.all = DictComponent({"build": build_task, "deploy": deploy_task, "run": run_task})

    def components(self) -> Dict[str, Component]:
        return self.__dict__


class AllComponents(ComponentGroup):

    def __init__(
        self,
        ferrite_dir: Path,
        source_dir: Path,
        target_dir: Path,
    ):
        self.host = HostComponents(
            ferrite_dir,
            source_dir,
            target_dir,
            HostAppPlatform(target_dir),
        )

        self.arm = CrossComponents(
            ferrite_dir,
            source_dir,
            target_dir,
            ArmAppPlatform("arm", target_dir),
            self.host,
        )

        self.aarch64 = CrossComponents(
            ferrite_dir,
            source_dir,
            target_dir,
            Aarch64AppPlatform("aarch64", target_dir),
            self.host,
        )

    def components(self) -> Dict[str, Component]:
        return self.__dict__


def make_components(ferrite_dir: Path, base_dir: Path, target_dir: Path) -> ComponentGroup:
    tree = AllComponents(ferrite_dir / "source", base_dir / "source", target_dir)
    tree._update_names()
    return tree
