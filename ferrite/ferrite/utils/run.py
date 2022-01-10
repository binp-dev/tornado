from __future__ import annotations
from typing import Any, List, Dict, Optional

import os
import sys
import logging
import subprocess

RunError = subprocess.CalledProcessError


def run(
    cmd: List[str],
    cwd: Optional[str] = None,
    add_env: Optional[Dict[str, str]] = None,
    capture: bool = False,
    quiet: bool = False,
) -> Optional[str]:
    logging.debug(f"run({cmd}, cwd={cwd})")
    env = dict(os.environ)
    if add_env:
        env.update(add_env)
        logging.debug(f"additional env: {add_env}")

    stdout = None
    if capture or quiet:
        stdout = subprocess.PIPE
    stderr = None
    if quiet:
        stderr = subprocess.STDOUT

    try:
        ret = subprocess.run(
            cmd,
            check=True,
            cwd=cwd,
            env=env,
            stdout=stdout,
            stderr=stderr,
        )
    except RunError as e:
        if capture or quiet:
            sys.stdout.buffer.write(e.output)
        raise

    if capture:
        return ret.stdout.decode("utf-8")
    else:
        return None


def capture(
    cmd: List[str],
    cwd: Optional[str] = None,
    add_env: Optional[Dict[str, str]] = None,
) -> str:
    result = run(cmd, cwd, add_env=add_env, capture=True)
    assert result is not None
    return result
