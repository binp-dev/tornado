from __future__ import annotations
from typing import Callable, Dict, List

from dataclasses import dataclass
from pathlib import Path
from multiprocessing import Process
from time import sleep

from ferrite.components.base import Component, Task, Context
from ferrite.components.rust import CargoBin
from ferrite.ioc.fakedev import test as fakedev_test


def gather(*fns: Callable[[], None]) -> None:
    ps = []
    for fn in fns:
        p = Process(target=fn)
        p.start()
        ps.append(p)
    for p in ps:
        p.join()


class Fakedev(Component):

    @dataclass
    class TestTask(Task):
        owner: Fakedev

        def run(self, ctx: Context) -> None:
            self.owner.test()

        def dependencies(self) -> List[Task]:
            return [self.owner.app.build_task]

    def __init__(
        self,
        app: CargoBin,
    ):
        self.app = app
        self.test_task = self.TestTask(self)

    def test(self) -> None:

        def run_app() -> None:
            sleep(1.0)
            self.app.run()

        gather(fakedev_test, run_app)

    def tasks(self) -> Dict[str, Task]:
        return {"test": self.test_task}
