from __future__ import annotations
import os
from manage.components.base import Component, Task, TaskWrapper, TaskList
from manage.components.epics.epics_base import EpicsBase
from manage.components.epics.ioc import AppIoc
from manage.components.mcu import Mcu
from manage.remote.tasks import RebootTask

class All(Component):
    def __init__(
        self,
        epics_base: EpicsBase,
        ioc: AppIoc,
        mcu: Mcu,
    ):
        super().__init__()
        self.epics_base = epics_base
        self.ioc = ioc
        self.mcu = mcu

        self.build_task = TaskWrapper(deps=[
            self.mcu.tasks()["build"],
            self.epics_base.tasks()["build_host"],
            self.epics_base.tasks()["build_cross"],
            self.ioc.tasks()["build_host"],
            self.ioc.tasks()["build_cross"],
        ])
        self.deploy_task = TaskWrapper(deps=[
            self.mcu.tasks()["deploy"],
            self.build_task,
            self.epics_base.tasks()["deploy"],
            self.ioc.tasks()["deploy"],
        ])
        self.deploy_and_reboot_task = TaskWrapper(
            RebootTask(),
            deps=[self.deploy_task],
        )
        self.run_task = TaskWrapper(
            self.ioc.tasks()["run"],
            deps=[self.deploy_and_reboot_task],
        )

    def tasks(self) -> dict[str, Task]:
        return {
            "build": self.build_task,
            "deploy": self.deploy_task,
            "deploy_and_reboot": self.deploy_and_reboot_task,
            "run": self.run_task,
        }
