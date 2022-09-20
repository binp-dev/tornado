from __future__ import annotations

from ferrite.utils.path import TargetPath
from ferrite.components.epics.epics_base import EpicsBaseCross, EpicsBaseHost
from ferrite.components.epics.ioc import IocCross, IocHost, HostBuildTask, CrossBuildTask
from ferrite.components.epics.app_ioc import AppIoc, B, AppBuildTask
from ferrite.components.app import AppBase

from example.info import path as self_path


class AbstractFrontend(AppIoc[B]):

    def __init__(self, epics_base: B, app: AppBase):
        super().__init__(
            [self_path / "source/ioc"],
            TargetPath("example/ioc"),
            epics_base,
            app,
        )


class FrontendHost(AbstractFrontend[EpicsBaseHost], IocHost):

    def BuildTask(self) -> _HostBuildTask:
        return _HostBuildTask(self)


class FrontendCross(AbstractFrontend[EpicsBaseCross], IocCross):

    def BuildTask(self) -> _CrossBuildTask:
        return _CrossBuildTask(self)


class _HostBuildTask(AppBuildTask[FrontendHost], HostBuildTask[FrontendHost]):
    pass


class _CrossBuildTask(AppBuildTask[FrontendCross], CrossBuildTask[FrontendCross]):
    pass
