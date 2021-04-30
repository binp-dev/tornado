from manage.components.toolchains import HostToolchain, AppToolchain, McuToolchain
from manage.components.freertos import Freertos
from manage.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from manage.components.mcu import Mcu
from manage.components.app import App
from manage.components.epics.ioc import AppIoc
from manage.components.all_ import AllHost, AllCross

def host_components(toolchain=HostToolchain()):
    epics_base = EpicsBaseHost(toolchain)
    app = App(epics_base, toolchain)
    ioc = AppIoc(epics_base, app, toolchain)
    all_ = AllHost(epics_base, app, ioc)
    return {
        "toolchain": toolchain,
        "epics_base": epics_base,
        "app": app,
        "ioc": ioc,
        "all": all_,
    }

def cross_components(app_toolchain, mcu_toolchain, host_components):
    epics_base = EpicsBaseCross(app_toolchain, host["epics_base"])
    app = App(epics_base, app_toolchain)
    ioc = AppIoc(epics_base, app, app_toolchain)
    freertos = Freertos()
    mcu = Mcu(freertos, mcu_toolchain)
    all_ = AllCross(epics_base, app, ioc, mcu)
    return {
        "app_toolchain": app_toolchain,
        "mcu_toolchain": mcu_toolchain,
        "freertos": freertos,
        "mcu": mcu,
        "epics_base": epics_base,
        "app": app,
        "ioc": ioc,
        "all": all_,
    }

host = host_components()
imx7 = cross_components(AppToolchain(), McuToolchain(), host)

components = {
    **{f"host_{k}": c for k, c in host.items()},
    **{f"imx7_{k}": c for k, c in imx7.items()},
}

for cname, comp in components.items():
    for tname, task in comp.tasks().items():
        if not task._name:
            task._name = f"{cname}.{tname}"
