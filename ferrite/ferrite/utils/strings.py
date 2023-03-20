from __future__ import annotations
from typing import Any, Mapping


def quote(text: str, char: str = '"') -> str:
    return char + text.replace("\\", "\\\\").replace(char, "\\" + char) + char
