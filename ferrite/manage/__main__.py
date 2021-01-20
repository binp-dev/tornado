import sys
import argparse
import logging
from manage.components.app import App
from manage.components.toolchains import AppToolchain, McuToolchain

logging.basicConfig(format='[%(levelname)s] %(message)s', level=logging.DEBUG)

app_toolchain = AppToolchain()
mcu_toolchain = McuToolchain()
components = {
    "app_toolchain": app_toolchain,
    "mcu_toolchain": mcu_toolchain,
    "app": App(app_toolchain)
}

if __name__ == "__main__":
    component_parser = argparse.ArgumentParser(
        description="Power supply controller software development automation tool",
        usage="python3 -m manage <component> <task> [options...]",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    component_parser.add_argument(
        "component", type=str,
        help="\n".join([
            "Component which task you want to run.",
            "Available components:",
            *[f"\t{name}" for name in components.keys()],
        ]),
    )
    component_name = component_parser.parse_args(sys.argv[1:2]).component
    component = components[component_name]

    task_parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
    )
    task_parser.add_argument(
        "task", type=str,
        help="\n".join([
            "Task to run.\nAvailable tasks for '{}':\n{}".format(
                component_name,
                "\n".join([f"\t{name}," for name in component.tasks().keys()])
            ),
        ]),
    )
    task_name = task_parser.parse_args(sys.argv[2:]).task
    task = component.tasks()[task_name]

    task.run({})
