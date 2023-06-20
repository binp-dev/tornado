from __future__ import annotations
from typing import Tuple, Dict

from pathlib import Path

from vortex.utils.path import TargetPath
from vortex.tasks.base import task, Context, Component, ComponentGroup
from vortex.tasks.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from vortex.tasks.rust import RustcHost, RustcCross

from .ioc import AppIoc, AppIocHost, AppIocCross
from .user import AbstractApp, AppReal, AppFake


class AppGroup(ComponentGroup):
    def components(self) -> Dict[str, Component]:
        return {"user": self.user, "ioc": self.ioc}

    @task
    def build(self, ctx: Context) -> None:
        self.ioc.build(ctx)

    @property
    def user(self) -> AbstractApp:
        raise NotImplementedError()

    @property
    def ioc(self) -> AppIoc:
        raise NotImplementedError()

    @staticmethod
    def _user_paths(src: Path, dst: TargetPath) -> Tuple[Path, TargetPath]:
        return (src / "user", dst / "user")

    @staticmethod
    def _ioc_paths(src: Path, dst: TargetPath) -> Tuple[Path, TargetPath]:
        return (src / "ioc", dst / "ioc")


class AppGroupHost(AppGroup):
    def __init__(self, rustc: RustcHost, epics_base: EpicsBaseHost, src: Path, dst: TargetPath) -> None:
        self._user = AppFake(rustc, *self._user_paths(src, dst))
        self._ioc = AppIocHost(*self._ioc_paths(src, dst), epics_base, dylibs=[self.user])

    @property
    def user(self) -> AppFake:
        return self._user

    @property
    def ioc(self) -> AppIocHost:
        return self._ioc


class AppGroupCross(AppGroup):
    def __init__(self, rustc: RustcCross, epics_base: EpicsBaseCross, src: Path, dst: TargetPath) -> None:
        assert rustc.cc is epics_base.cc
        self.cc = rustc.cc
        self.rustc = rustc
        self._user = AppReal(self.rustc, *self._user_paths(src, dst))
        self._ioc = AppIocCross(*self._ioc_paths(src, dst), epics_base, dylibs=[self.user])

    def components(self) -> Dict[str, Component]:
        return {
            **super().components(),
            "gcc": self.cc,
            "rustc": self.rustc,
        }

    @task
    def deploy(self, ctx: Context) -> None:
        self.ioc.deploy(ctx)

    @task
    def run(self, ctx: Context) -> None:
        self.ioc.run(ctx)

    @property
    def user(self) -> AppReal:
        return self._user

    @property
    def ioc(self) -> AppIocCross:
        return self._ioc
