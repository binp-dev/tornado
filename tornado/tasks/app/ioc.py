from __future__ import annotations

from datetime import datetime, timezone

from vortex.utils.run import capture
from vortex.utils.files import substitute
from vortex.tasks.base import Context
from vortex.tasks.epics.ioc import IocHost, IocCross, AbstractIocWithLibs


class AbstractAppIoc(AbstractIocWithLibs):
    @property
    def name(self) -> str:
        return "Tornado"

    def _configure(self, ctx: Context) -> None:
        super()._configure(ctx)

        substitute(
            [("^\\s*#*(\\s*APP_ARCH\\s*=).*$", f"\\1 {self.arch}")],
            ctx.target_path / self.build_dir / "configure/CONFIG_SITE.local",
        )

    def _post_install(self, ctx: Context) -> None:
        super()._post_install(ctx)

        hash = capture(["git", "rev-parse", "--short", "HEAD"])
        dirty = "-dirty" if capture(["git", "status", "--short"]) != "" else ""

        version = f"0.4.0-alpha.0-g{hash}{dirty}"
        date = datetime.now(timezone.utc).astimezone().strftime("%Y-%m-%d %H:%M:%S %z")

        with open(ctx.target_path / self.install_dir / f"iocBoot/ioc{self.name}/envDebug", "w") as f:
            f.write(f'epicsEnvSet("VERSION","{version}")\n')
            f.write(f'epicsEnvSet("BUILD_DATE","{date}")\n')

class AppIocHost(AbstractAppIoc, IocHost):
    pass


class AppIocCross(AbstractAppIoc, IocCross):
    pass
