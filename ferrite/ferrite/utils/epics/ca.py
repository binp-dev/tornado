from __future__ import annotations
from math import fabs
from sqlite3 import OptimizedUnicode
from typing import Any, Dict, List, Optional, Union

import os
import time
import logging
from subprocess import Popen
from pathlib import Path

from ferrite.utils.run import run, capture


def _env() -> Dict[str, str]:
    return {
        "EPICS_CA_AUTO_ADDR_LIST": "NO",
        "EPICS_CA_ADDR_LIST": "127.0.0.1",
    }


def get(prefix: Path, pv: str, array: bool = False) -> Union[float, List[int]]:
    logging.debug(f"caget {pv} ...")
    out = capture([prefix / "caget", "-t", "-f 3", pv], add_env=_env())
    logging.debug(f"  {out}")
    
    if array:
        out = out.split()[1:]
        out = list(map(int, out))
        print(out)
        return out
    
    return float(out)


def put(prefix: Path, pv: str, value: Union[int, float, List[int]], array: bool = False) -> None:
    logging.debug(f"caput {pv} {value} ...")

    args: List[str | Path] = [prefix / "caput", "-t"]
    if not array:
        assert isinstance(value, int) or isinstance(value, float)
        args += [pv, str(value)]
    else:
        assert isinstance(value, List)
        args += ["-a", pv, str(len(value))] + [str(v) for v in value]

    run(
        args,
        add_env=_env(),
        quiet=True,
    )
    logging.debug("  done")


class Repeater:

    def __init__(self, prefix: Path):
        self.proc: Optional[Popen[bytes]] = None
        self.prefix = prefix

    def __enter__(self) -> None:
        logging.debug("starting caRepeater ...")
        env = dict(os.environ)
        env.update(_env())
        self.proc = Popen(
            [self.prefix / "caRepeater"],
            env=env,
        )
        time.sleep(1)
        logging.debug("caRepeater started")

    def __exit__(self, *args: Any) -> None:
        logging.debug("terminating caRepeater ...")
        assert self.proc is not None
        self.proc.terminate()
        logging.debug("caRepeater terminated")
