from __future__ import annotations
from typing import Dict, Union

import re
from pathlib import Path

_redef = re.compile("^\s*#define\s+(\S+)\s+(.*)$")
_recom = re.compile("//.*$")

DefVal = Union[int, float, str, None]


def read_defs(path: Path) -> Dict[str, DefVal]:
    pairs = {}
    for l in open(path, "r"):
        m = re.search(_redef, re.sub(_recom, "", l).strip())
        if m is None:
            continue

        k = m.group(1)
        s = m.group(2).strip()
        assert isinstance(k, str) and isinstance(s, str)

        v: DefVal
        try:
            v = int(s, 0)
        except ValueError:
            try:
                v = float(s)
            except ValueError:
                if len(s) > 0:
                    v = s
                else:
                    v = None
        assert k not in pairs
        pairs[k] = v

    return pairs
