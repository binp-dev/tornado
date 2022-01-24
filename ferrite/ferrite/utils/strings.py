from __future__ import annotations
from typing import Any, Mapping


def quote(text: str, char: str = '"') -> str:
    return char + text.replace("\\", "\\\\").replace(char, "\\" + char) + char


def try_format(src: str, **kwargs: Any) -> str:
    rep = {}
    for k, v in kwargs.items():
        if ("{" + k + "}") in src:
            rep[k] = v
    return src.format(**rep)
