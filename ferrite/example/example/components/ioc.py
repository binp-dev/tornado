from __future__ import annotations
from typing import List

from pathlib import Path

from ferrite.components.epics.epics_base import EpicsBaseCross, EpicsBaseHost
from ferrite.components.epics.ioc import IocCross, IocHost
from ferrite.components.epics.app_ioc import AbstractAppIoc
from ferrite.components.app import AppBase


class AppIocHost(AbstractAppIoc, IocHost):

    def __init__(
        self,
        ferrite_dir: Path,
        source_dir: Path,
        target_dir: Path,
        epics_base: EpicsBaseHost,
        app: AppBase,
    ):
        self.app = app

        super().__init__(
            [ferrite_dir / "ioc", source_dir / "ioc"],
            target_dir / "ioc_host",
            epics_base,
            app,
        )

    class BuildTask(AbstractAppIoc.BuildTask, IocHost.BuildTask):
        pass


class AppIocCross(AbstractAppIoc, IocCross):

    def __init__(
        self,
        ferrite_dir: Path,
        source_dir: Path,
        target_dir: Path,
        epics_base: EpicsBaseCross,
        app: AppBase,
    ):
        self.app = app

        super().__init__(
            [ferrite_dir / "ioc", source_dir / "ioc"],
            target_dir / f"ioc_{epics_base.cc.name}",
            epics_base,
            app,
        )

    class BuildTask(AbstractAppIoc.BuildTask, IocCross.BuildTask):
        pass
