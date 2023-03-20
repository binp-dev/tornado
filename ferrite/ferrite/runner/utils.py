from __future__ import annotations
from typing import Callable, Sequence, List, TypeVar

from pathlib import Path

from ferrite.utils.path import PathLike


def rename_mkdir(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    src.rename(dst)


def remove_empty_tree(path: Path) -> None:
    contents = list(path.iterdir())
    if len(contents) != 0:
        for p in contents:
            remove_empty_tree(p)
    path.rmdir()


T = TypeVar("T")
P = TypeVar("P", bound=PathLike)


def filter_parents(items: Sequence[T], path: Callable[[T], P]) -> List[T]:
    filtered: List[T] = []
    for x in items:
        filtered = [y for y in filtered if path(x) not in path(y).parents]
        if all([path(y) != path(x) and path(y) not in path(x).parents for y in filtered]):
            filtered.append(x)
    return filtered
