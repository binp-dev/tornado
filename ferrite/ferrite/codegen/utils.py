from __future__ import annotations
from typing import List, TypeVar

import re
import zlib


def hash_str(text: str) -> str:
    hash = zlib.adler32(text.encode("utf-8"))
    return f"{hash:0{8}x}"


def to_ident(text: str, _pat: re.Pattern[str] = re.compile("[^a-zA-Z0-9_]")) -> str:
    ident = re.sub(_pat, "_", text)
    assert isinstance(ident, str)
    if ident != text:
        ident += f"_{hash_str(text)}"
    return ident


def is_power_of_2(n: int) -> bool:
    return bool(n & (n - 1) == 0) and n != 0


def ceil_to_power_of_2(n: int) -> int:
    return 1 << (n - 1).bit_length()


def upper_multiple(x: int, m: int) -> int:
    return ((x - 1) // m + 1) * m


def lower_multiple(x: int, m: int) -> int:
    return (x // m) * m


def pad_bytes(b: bytes, m: int, c: bytes = b'\x00') -> bytes:
    return b + (c * upper_multiple(len(b), m))


T = TypeVar('T')


def list_join(lists: List[List[T]], sep: List[T] = []) -> List[T]:
    result = []
    for i, l in enumerate(lists):
        if i > 0:
            result.extend(sep)
        result.extend(l)
    return result


def indent(text: List[str], count: int = 1) -> List[str]:
    return ["    " * count + line for line in text]
