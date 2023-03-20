from __future__ import annotations
from math import fabs
from sqlite3 import OptimizedUnicode
from typing import Any, Dict, List, Optional, Union

import os
import time
from subprocess import Popen
from pathlib import Path

from ferrite.utils.run import run, capture

import logging

logger = logging.getLogger(__name__)


def local_env() -> Dict[str, str]:
    return {
        "EPICS_CA_AUTO_ADDR_LIST": "NO",
        "EPICS_CA_ADDR_LIST": "127.0.0.1",
    }


def _get_str(prefix: Path, pv: str) -> str:
    logger.debug(f"caget {pv} ...")
    out = capture([prefix / "caget", "-t", "-f 3", pv])
    logger.debug(f"  {out}")
    return out


def get(prefix: Path, pv: str) -> float:
    return float(_get_str(prefix, pv))


def get_array(prefix: Path, pv: str) -> List[float]:
    spl = _get_str(prefix, pv).strip().split()
    arr_len, str_arr = int(spl[0]), spl[1:]
    assert arr_len == len(str_arr)
    return [float(x) for x in str_arr]


def put(prefix: Path, pv: str, value: int | float) -> None:
    logger.debug(f"caput {pv} {value} ...")
    run([prefix / "caput", "-t", pv, str(value)], quiet=True)
    logger.debug("  done")


def put_array(prefix: Path, pv: str, value: List[int] | List[float]) -> None:
    logger.debug(f"caput {pv} {value} ...")

    args: List[str | Path] = [prefix / "caput", "-t", "-a", pv, str(len(value))]
    args.extend([str(v) for v in value])

    run(args, quiet=True)
    logger.debug("  done")


class Repeater:

    def __init__(self, base_dir: Path, arch: str, env: Dict[str, str] = {}):
        self.proc: Optional[Popen[bytes]] = None
        self.base_dir = base_dir
        self.arch = arch
        self._env = env

    def env(self) -> Dict[str, str]:
        return {
            **dict(os.environ),
            **self._env,
            "LD_LIBRARY_PATH": str(self.base_dir / "lib" / self.arch),
        }

    def __enter__(self) -> None:
        logger.debug("starting caRepeater ...")

        self.proc = Popen(
            [self.base_dir / "bin" / self.arch / "caRepeater"],
            env=self.env(),
        )
        time.sleep(1)
        logger.debug("caRepeater started")

    def __exit__(self, *args: Any) -> None:
        logger.debug("terminating caRepeater ...")
        assert self.proc is not None
        self.proc.terminate()
        logger.debug("caRepeater terminated")
