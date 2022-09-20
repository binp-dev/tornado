from __future__ import annotations
from typing import List, Protocol, Sequence, TypeVar, Any

from pathlib import PurePath, Path

Self = TypeVar("Self")


class PathLike(Protocol):

    def __truediv__(self: Self, rhs: str | PurePath) -> Self:
        ...

    def __eq__(self: Self, other: Any) -> bool:
        ...

    @property
    def parent(self: Self) -> Self:
        ...

    @property
    def parents(self: Self) -> Sequence[Self]:
        ...


class TargetPath:
    "Path relative to target directory."

    def __init__(self, *args: str | PurePath | TargetPath) -> None:
        self._inner: PurePath = PurePath(*[a._inner if isinstance(a, TargetPath) else a for a in args])

    def __truediv__(self, rhs: str | PurePath | TargetPath) -> TargetPath:
        return TargetPath(self, rhs)

    def __rtruediv__(self, lhs: Path) -> Path:
        return lhs / self._inner

    def __eq__(self, other: Any) -> bool:
        assert isinstance(other, TargetPath)
        return self._inner == other._inner

    def __str__(self) -> str:
        return str(self._inner)

    @property
    def parent(self) -> TargetPath:
        return TargetPath(self._inner.parent)

    @property
    def parents(self) -> List[TargetPath]:
        return [TargetPath(p) for p in self._inner.parents]


def _test(path: PathLike) -> None:
    pass


_test(PurePath())
_test(Path())
_test(TargetPath())
