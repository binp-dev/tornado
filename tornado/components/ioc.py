from __future__ import annotations

from pathlib import Path

from ferrite.components.epics.epics_base import EpicsBaseCross, EpicsBaseHost
from ferrite.components.epics.ioc import IocCross, IocHost, HostBuildTask, CrossBuildTask
from ferrite.components.epics.app_ioc import AppIoc, B, AppBuildTask
from ferrite.components.app import AppBase


class AbstractAppIoc(AppIoc[B]):

    def __init__(
        self,
        ferrite_dir: Path,
        source_dir: Path,
        target_dir: Path,
        epics_base: B,
        app: AppBase,
    ):
        super().__init__(
            [ferrite_dir / "ioc", source_dir / "ioc"],
            target_dir / "ioc",
            epics_base,
            app,
        )


class AppIocHost(AbstractAppIoc[EpicsBaseHost], IocHost):

    def BuildTask(self) -> _HostBuildTask:
        return _HostBuildTask(self)


class AppIocCross(AbstractAppIoc[EpicsBaseCross], IocCross):

    def BuildTask(self) -> _CrossBuildTask:
        return _CrossBuildTask(self)


class _HostBuildTask(AppBuildTask[AppIocHost], HostBuildTask[AppIocHost]):
    pass


class _CrossBuildTask(AppBuildTask[AppIocCross], CrossBuildTask[AppIocCross]):
    pass
