from __future__ import annotations

import argparse
from pathlib import Path

import ferrite.manage.cli as cli
from ferrite.components.tree import make_components
from ferrite.info import path as self_path

if __name__ == "__main__":
    target_dir = self_path / "target"
    target_dir.mkdir(exist_ok=True)

    components = make_components(self_path, target_dir)

    parser = argparse.ArgumentParser(
        description="Power supply controller software development automation library",
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
    cli.run_with_params(target_dir, params)
