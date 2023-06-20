from __future__ import annotations

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


class AppIocHost(AbstractAppIoc, IocHost):
    pass


class AppIocCross(AbstractAppIoc, IocCross):
    pass
