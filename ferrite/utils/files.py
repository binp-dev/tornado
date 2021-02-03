from __future__ import annotations
from typing import Callable
import re
import logging
import shutil 

def match(pat, src):
    logging.debug("matching '%s': '%s':" % (src, pat))

    with open(src, 'r') as file:
        data = file.read()
    
    return re.match(pat, flags=re.M)

def substitute(rep, src, dst=None, force=False):
    if dst is None:
        dst = src
    
    logging.debug("substituting '%s' -> '%s':" % (src, dst))

    with open(src, 'r') as file:
        data = file.read()

    new_data = data
    for s, d in rep:
        new_data = re.sub(s, d, new_data, flags=re.M)

    if force or new_data != data:
        logging.debug(f"writing file '{dst}'")
        with open(dst, 'w') as file:
            file.write(new_data)
    else:
        logging.debug(f"file unchanged '{dst}'")

def _inverse_ignore_patterns(
    ignore_patterns: Callable[[str, list[str]], list[str]],
) -> Callable[[str, list[str]], list[str]]:
    def allow_patterns(path: str, names: list[str]) -> list[str]:
        return list(set(names) - set(ignore_patterns(path, names)))
    return allow_patterns

def allow_patterns(*patterns):
    return _inverse_ignore_patterns(shutil.ignore_patterns(*patterns))
