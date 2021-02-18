from manage.components.toolchains import AppToolchain, McuToolchain
from manage.components.freertos import Freertos
from manage.components.epics.epics_base import EpicsBase
from manage.components.mcu import Mcu
from manage.components.app import App
from manage.components.epics.ioc import AppIoc
from manage.components.all_ import All

mcu_toolchain = McuToolchain()
app_toolchain = AppToolchain()
freertos = Freertos()
epics_base = EpicsBase(app_toolchain)
mcu = Mcu(freertos, mcu_toolchain)
app = App(epics_base, app_toolchain)
ioc = AppIoc(epics_base, app, app_toolchain)
all_ = All(epics_base, app, ioc, mcu)

components = {
    "mcu_toolchain": mcu_toolchain,
    "app_toolchain": app_toolchain,
    "freertos": freertos,
    "epics_base": epics_base,
    "mcu": mcu,
    "app": app,
    "ioc": ioc,
    "all": all_,
}

for cname, comp in components.items():
    for tname, task in comp.tasks().items():
        if not task._name:
            task._name = f"{cname}.{tname}"
