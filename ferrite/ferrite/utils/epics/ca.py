from __future__ import annotations
from typing import Any, Dict, Iterable, List, Optional, Union

import os
import time
from subprocess import Popen

from ferrite.utils.run import run, capture


def _env() -> Dict[str, str]:
    return {
        "EPICS_CA_AUTO_ADDR_LIST": "NO",
        "EPICS_CA_ADDR_LIST": "127.0.0.1",
    }


def get(prefix: str, pv: str) -> float:
    print(f"caget {pv} ...")
    out = capture([os.path.join(prefix, "caget"), "-t", "-f 3", pv], add_env=_env()).strip()
    print(f"  {out}")
    return float(out)


def put(prefix: str, pv: str, value: Union[int, float, List[float]], array: bool = False) -> None:
    print(f"caput {pv} {value} ...")

    args = [os.path.join(prefix, "caput"), "-t"]
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
    print("  done")


class Repeater:

    def __init__(self, prefix: str):
        self.proc: Optional[Popen[bytes]] = None
        self.prefix = prefix

    def __enter__(self) -> None:
        print("starting caRepeater ...")
        env = dict(os.environ)
        env.update(_env())
        self.proc = Popen(
            [os.path.join(self.prefix, "caRepeater")],
            env=env,
        )
        time.sleep(1)
        print("caRepeater started")

    def __exit__(self, *args: Any) -> None:
        print("terminating caRepeater ...")
        assert self.proc is not None
        self.proc.terminate()
        print("caRepeater terminated")
