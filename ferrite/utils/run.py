from __future__ import annotations
import os
import logging
import subprocess

RunError = subprocess.CalledProcessError

def run(
    cmd: list[str],
    add_env: dict[str, str] = None,
    capture: bool = False,
    log: bool = True,
    **kwargs,
) -> str:
    if log:
        logging.debug(f"run({cmd}, {kwargs})")
    env = dict(os.environ)
    if add_env:
        env.update(add_env)
        if log:
            logging.info("additional env: {}".format(add_env))
    params = {}
    if capture:
        params["stdout"] = subprocess.PIPE
    ret = subprocess.run(cmd, check=True, env=env, **params, **kwargs)
    if capture:
        return ret.stdout.decode("utf-8")

def quote(text: str, char: str = '"'):
    return char + text.replace(r"/", r"//").replace(char, r"/" + char) + char
