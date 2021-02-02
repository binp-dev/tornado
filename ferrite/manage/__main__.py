import sys
import argparse
import logging
from manage.components.toolchains import AppToolchain, McuToolchain
from manage.components.freertos import Freertos
from manage.components.epics.epics_base import EpicsBase
from manage.components.mcu import Mcu
from manage.components.app import App
from manage.components.epics.ioc import AppIoc

logging.basicConfig(format='[%(levelname)s] %(message)s', level=logging.DEBUG)

mcu_toolchain = McuToolchain()
app_toolchain = AppToolchain()
freertos = Freertos()
epics_base = EpicsBase(app_toolchain)
mcu = Mcu(freertos, mcu_toolchain)
app = App(epics_base, app_toolchain)
ioc = AppIoc(epics_base, app, app_toolchain)

components = {
    "mcu_toolchain": mcu_toolchain,
    "app_toolchain": app_toolchain,
    "freertos": freertos,
    "epics_base": epics_base,
    "mcu": mcu,
    "app": app,
    "ioc": ioc,
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Power supply controller software development automation tool",
        usage="python3 -m manage <component>.<task> [options...]",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    available_components_text = "\n".join([
        "Available components:",
        *[f"\t{name}" for name in components.keys()],
    ])
    parser.add_argument(
        "action", type=str, metavar="<component>.<task>",
        help="\n".join([
            "Component and task you want to run.",
            available_components_text,
        ]),
    )
    args = parser.parse_args()

    names = args.action.split(".")
    component_name = names[0]
    try:
        component = components[component_name]
    except KeyError:
        print("\n".join([
            f"Unknown component '{component_name}'.",
            available_components_text
        ]))
        exit(1)

    available_tasks_text = "\n".join([
        f"Available tasks for component '{component_name}':",
        *[f"\t{name}" for name in component.tasks().keys()],
    ])
    if len(names) == 1:
        print("\n".join([
            "No task provided.",
            available_tasks_text,
        ]))
        exit(1)
    elif len(names) == 2:
        task_name = names[1]
        try:
            task = component.tasks()[task_name]
        except KeyError:
            print("\n".join([
                f"Unknown task '{task_name}'.",
                available_tasks_text,
            ]))
            exit(1)
    else:
        print("Bad action syntax. Expected format: '<component>.<task>'.")
        exit(1)

    task.run_with_dependencies(None)
