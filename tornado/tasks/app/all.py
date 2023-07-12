from __future__ import annotations

from pathlib import Path, PurePosixPath

from vortex.utils.path import TargetPath
from vortex.output.base import Device
from vortex.tasks.base import task, Context, ComponentGroup
from vortex.tasks.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from vortex.tasks.rust import RustcHost, RustcCross

from .ioc import AppIocHost, AppIocCross
from .user import AppReal, AppFake


class AppGroupHost(ComponentGroup):
    def __init__(self, rustc: RustcHost, epics_base: EpicsBaseHost, src: Path, dst: TargetPath) -> None:
        self.user = AppFake(rustc, src / "user", dst / "user")
        self.ioc = AppIocHost(src / "ioc", dst / "ioc", epics_base, dylibs=[self.user])

    @task
    def build(self, ctx: Context) -> None:
        self.ioc.build(ctx)

    @task
    def run(self, ctx: Context) -> None:
        self.ioc.run(ctx)


class AppGroupCross(ComponentGroup):
    def __init__(self, rustc: RustcCross, epics_base: EpicsBaseCross, src: Path, dst: TargetPath) -> None:
        self.src = src
        assert rustc.cc is epics_base.cc
        self.cc = rustc.cc
        self.rustc = rustc
        self.user = AppReal(self.rustc, src / "user", dst / "user")
        self.ioc = AppIocCross(src / "ioc", dst / "ioc", epics_base, dylibs=[self.user])

    @task
    def build(self, ctx: Context) -> None:
        self.ioc.build(ctx)

    @task
    def deploy(self, ctx: Context) -> None:
        assert ctx.output is not None
        ctx.output.mkdir(PurePosixPath("/opt"), exist_ok=True)

        self.ioc.deploy(ctx)

        ctx.output.copy(self.src / "misc/run_ioc.sh", PurePosixPath("/opt/run_ioc.sh"))

        systemd = PurePosixPath("/etc/systemd/system")
        wants = "multi-user.target.wants"
        service = "ioc.service"
        ctx.output.mkdir(systemd / wants, exist_ok=True, recursive=True)
        ctx.output.copy(self.src / "misc" / service, systemd / service)
        ctx.output.link(PurePosixPath(systemd / wants / service), systemd / service)

    @task
    def restart(self, ctx: Context) -> None:
        assert isinstance(ctx.output, Device)
        ctx.output.run(["systemctl", "daemon-reload"])
        ctx.output.run(["systemctl", "restart", "ioc"])
