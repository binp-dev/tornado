from __future__ import annotations

import argparse
from pathlib import Path

import ferrite.manage.cli as cli

from example.components.tree import make_components

if __name__ == "__main__":
    base_dir = Path.cwd()
    target_dir = base_dir / "target"
    target_dir.mkdir(exist_ok=True)
    ferrite_dir = (base_dir / "..").resolve()

    components = make_components(ferrite_dir, base_dir, target_dir)

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

    cli.setup_logging(params)
    cli.run_with_params(params)
