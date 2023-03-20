from __future__ import annotations

import argparse

import vortex.manage.cli as cli

from tornado.tasks.tree import AllTasks
from tornado.manage.info import path as self_path

if __name__ == "__main__":
    tasks = AllTasks()

    parser = argparse.ArgumentParser(
        description="Tornado power supply controller development automation tool",
        usage="python -m tornado.manage <component>.<task> [options...]",
    )
    cli.add_parser_args(parser, tasks)

    args = parser.parse_args()

    try:
        params = cli.read_run_params(args, tasks, self_path)
    except cli.ReadRunParamsError as e:
        print(e)
        exit(1)

    cli.setup_logging(params, ["vortex", "tornado"])
    cli.run_with_params(params)
