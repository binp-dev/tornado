import manage.components.toolchains as toolchains
from manage.components.freertos import FreertosImx7, FreertosImx8mn
from manage.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from manage.components.mcu import Mcu
from manage.components.ipp import Ipp
from manage.components.app import App
from manage.components.epics.ioc import AppIoc
from manage.components.all_ import AllHost, AllCross

def host_components(toolchain):
    epics_base = EpicsBaseHost(toolchain)
    ipp = Ipp(toolchain)
    app = App(epics_base, toolchain)
    ioc = AppIoc(epics_base, app, toolchain)
    all_ = AllHost(epics_base, ipp, app, ioc)
    return {
        "toolchain": toolchain,
        "epics_base": epics_base,
        "ipp": ipp,
        "app": app,
        "ioc": ioc,
        "all": all_,
    }

def cross_components(host_components, app_toolchain, mcu_toolchain, freertos):
    epics_base = EpicsBaseCross(app_toolchain, host_components["epics_base"])
    app = App(epics_base, app_toolchain)
    ioc = AppIoc(epics_base, app, app_toolchain)
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

host = host_components(
    toolchains.HostToolchain()
)
imx7 = cross_components(
    host,
    toolchains.AppToolchainImx7(),
    toolchains.McuToolchainImx7(),
    FreertosImx7(),
)
imx8mn = cross_components(
    host,
    toolchains.AppToolchainImx8mn(),
    toolchains.McuToolchainImx8mn(),
    FreertosImx8mn(),
)

components = {
    **{f"host_{k}": c for k, c in host.items()},
    **{f"imx7_{k}": c for k, c in imx7.items()},
    **{f"imx8mn_{k}": c for k, c in imx8mn.items()},
}

for cname, comp in components.items():
    for tname, task in comp.tasks().items():
        if not task._name:
            task._name = f"{cname}.{tname}"
