from __future__ import annotations

import argparse
from pathlib import Path

import ferrite.manage.cli as cli

from tornado.manage.tree import make_components

if __name__ == "__main__":
    base_dir = Path.cwd()
    ferrite_dir = base_dir / "ferrite"
    target_dir = base_dir / "target"
    target_dir.mkdir(exist_ok=True)

    components = make_components(base_dir, ferrite_dir, target_dir)

    parser = argparse.ArgumentParser(
        description="Tornado power supply controller development automation tool",
        usage="python -m tornado.manage <component>.<task> [options...]",
    )
    cli.add_parser_args(parser, components)

    args = parser.parse_args()

    try:
        params = cli.read_run_params(args, components)
    except cli.ReadRunParamsError as e:
        print(e)
        exit(1)

    cli.setup_logging(params, ["ferrite", "tornado"])
    cli.run_with_params(params)
