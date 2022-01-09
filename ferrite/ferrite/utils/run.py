from __future__ import annotations
import os
import sys
import logging
import subprocess

RunError = subprocess.CalledProcessError

def run(
    cmd: list[str],
    add_env: dict[str, str] = None,
    capture: bool = False,
    quiet: bool = False,
    log: bool = True,
    **kwargs,
) -> str:
    if log:
        logging.debug(f"run({cmd}, {kwargs})")
    env = dict(os.environ)
    if add_env:
        env.update(add_env)
        if log:
            logging.debug("additional env: {}".format(add_env))
    params = {}

    if capture or quiet:
        params["stdout"] = subprocess.PIPE
    if quiet:
        params["stderr"] = subprocess.STDOUT

    try:
        ret = subprocess.run(cmd, check=True, env=env, **params, **kwargs)
    except RunError as e:
        if capture or quiet:
            sys.stdout.buffer.write(e.output)
        raise

    if capture:
        return ret.stdout.decode("utf-8")
