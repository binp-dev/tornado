from __future__ import annotations

import argparse

from ferrite.manage.tree import COMPONENTS
from ferrite.manage.cli import add_parser_args, read_run_params, ReadRunParamsError, run_with_params

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Power supply controller software development automation tool",
        usage="python -m ferrite.manage <component>.<task> [options...]",
    )
    add_parser_args(parser, COMPONENTS)

    args = parser.parse_args()

    try:
        params = read_run_params(args, COMPONENTS)
    except ReadRunParamsError as e:
        print(e)
        exit(1)

    run_with_params(params)
