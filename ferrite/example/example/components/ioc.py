from __future__ import annotations
from typing import List

from pathlib import Path

from ferrite.components.epics.epics_base import AbstractEpicsBase
from ferrite.components.epics.ioc import IocCross, IocHost
from ferrite.components.epics.app_ioc import AbstractAppIoc
from ferrite.components.app import AppBase


class AppIocBase(AbstractAppIoc):

    def __init__(
        self,
        ferrite_dir: Path,
        source_dir: Path,
        target_dir: Path,
        epics_base: AbstractEpicsBase,
        app: AppBase,
    ):
        self.app = app

        super().__init__(
            [ferrite_dir / "ioc", source_dir / "ioc"],
            target_dir / "ioc",
            epics_base,
            app,
        )


class AppIocHost(AppIocBase, IocHost):

    class BuildTask(AppIocBase.BuildTask, IocHost.BuildTask):
        pass


class AppIocCross(AppIocBase, IocCross):

    class BuildTask(AppIocBase.BuildTask, IocCross.BuildTask):
        pass
