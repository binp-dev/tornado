from __future__ import annotations

import argparse
import logging

import ferrite.manage.cli as cli

from example.components.tree import make_components

if __name__ == "__main__":
    components = make_components()

    parser = argparse.ArgumentParser(
        description="Power supply controller software development automation tool",
        usage="python -m ferrite.manage <task> [options...]",
    )
    cli.add_parser_args(parser, components)

    args = parser.parse_args()

    try:
        params = cli.read_run_params(args, components)
    except cli.ReadRunParamsError as e:
        print(e)
        exit(1)

    cli.setup_logging(params, ["ferrite", "example"], level=logging.INFO)
    cli.run_with_params(params)
