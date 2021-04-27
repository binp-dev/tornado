from manage.components.toolchains import HostToolchain, AppToolchain, McuToolchain
from manage.components.freertos import Freertos
from manage.components.epics.epics_base import EpicsBaseHost, EpicsBaseCross
from manage.components.mcu import Mcu
from manage.components.app import App
from manage.components.epics.ioc import AppIoc
from manage.components.all_ import All

host_toolchain = HostToolchain()
#mcu_toolchain = McuToolchain()
app_toolchain = AppToolchain()
#freertos = Freertos()
epics_base_host = EpicsBaseHost(host_toolchain)
epics_base_cross = EpicsBaseCross(app_toolchain, epics_base_host)
#mcu = Mcu(freertos, mcu_toolchain)
app_host = App(epics_base_host, host_toolchain)
app_cross = App(epics_base_cross, app_toolchain)
ioc_host = AppIoc(epics_base_host, app_host, host_toolchain)
ioc_cross = AppIoc(epics_base_cross, app_cross, app_toolchain)
#all_ = All(epics_base, app, ioc, mcu)

components = {
    "host_toolchain": host_toolchain,
    "app_toolchain": app_toolchain,
    "epics_base_host": epics_base_host,
    "epics_base_cross": epics_base_cross,
    "app_host": app_host,
    "app_cross": app_cross,
    "ioc_host": ioc_host,
    "ioc_cross": ioc_cross,
}

for cname, comp in components.items():
    for tname, task in comp.tasks().items():
        if not task._name:
            task._name = f"{cname}.{tname}"
