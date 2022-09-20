from __future__ import annotations
from typing import Dict

from pathlib import Path

from ferrite.components.base import TaskList, TaskWrapper, Component, DictComponent, ComponentGroup
from ferrite.components.platforms.base import AppPlatform
from ferrite.components.platforms.host import HostPlatform
from ferrite.components.platforms.arm import Aarch64AppPlatform, ArmAppPlatform
from ferrite.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross

from example.components.app import App
from example.components.frontend import FrontendHost, FrontendCross
from example.components.backend import TestBackend
from example.components.protocol import Protocol


class HostComponents(ComponentGroup):

    def __init__(self, platform: HostPlatform) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.epics_base = EpicsBaseHost(self.gcc)
        self.protocol = Protocol(self.rustc)
        self.app = App(self.rustc, self.protocol)
        self.ioc = FrontendHost(self.epics_base, self.app)
        self.backend = TestBackend(self.ioc, self.protocol)
        self.all = DictComponent({
            "build": TaskList([self.epics_base.install_task, self.app.build_task, self.ioc.install_task]),
            "test": TaskList([self.protocol.test_task, self.app.test_task, self.backend.test_task]),
        })

    def components(self) -> Dict[str, Component]:
        return self.__dict__


class CrossComponents(ComponentGroup):

    def __init__(self, platform: AppPlatform, host: HostComponents) -> None:
        self.gcc = platform.gcc
        self.rustc = platform.rustc
        self.epics_base = EpicsBaseCross(self.gcc, host.epics_base)
        self.app = App(self.rustc, host.protocol)
        self.ioc = FrontendCross(self.epics_base, self.app)

        build_task = TaskList([self.epics_base.install_task, self.ioc.install_task])
        deploy_task = TaskList([self.epics_base.deploy_task, self.ioc.deploy_task])
        run_task = TaskWrapper(self.ioc.run_task, [deploy_task])
        self.all = DictComponent({"build": build_task, "deploy": deploy_task, "run": run_task})

    def components(self) -> Dict[str, Component]:
        return self.__dict__


class AllComponents(ComponentGroup):

    def __init__(self) -> None:
        self.host = HostComponents(HostPlatform())
        self.arm = CrossComponents(ArmAppPlatform("arm"), self.host)
        self.aarch64 = CrossComponents(Aarch64AppPlatform("aarch64"), self.host)

    def components(self) -> Dict[str, Component]:
        return self.__dict__


def make_components() -> ComponentGroup:
    tree = AllComponents()
    tree._update_names()
    return tree
