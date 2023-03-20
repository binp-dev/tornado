from __future__ import annotations
from typing import List, Tuple, ClassVar

import math
from dataclasses import dataclass


@dataclass
class ProgressBar:
    length: int = 64
    chars: str = "#."

    @property
    def progress(self) -> float:
        raise NotImplementedError()

    def _progress_line(self) -> str:
        part = self.progress * self.length
        line = []
        for i in range(self.length):
            idx = min(max(int(len(self.chars) * (i + 1 - part)), 0), len(self.chars) - 1)
            line.append(self.chars[idx])
        return "".join(line)

    def _text_current(self) -> str:
        raise NotImplementedError()

    def _text_total(self) -> str:
        raise NotImplementedError()

    def print(self) -> None:
        print(
            f"\r[{self._progress_line()}] {self._text_current()} of {self._text_total()}",
            end="",
            flush=True,
        )

    def _should_update(self, progress: float) -> bool:
        return abs(progress - self.progress) * self.length * len(self.chars) > 1.0


@dataclass
class DownloadBar(ProgressBar):
    PREFIXES: ClassVar[List[str]] = ["  ", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"]

    current_bytes: int = 0
    total_bytes: int = 0

    @property
    def progress(self) -> float:
        if self.total_bytes <= 0:
            return 0.0
        return self.current_bytes / self.total_bytes

    @staticmethod
    def _units(value: float) -> Tuple[float, int]:
        power = 0
        while abs(value) >= 1024.0:
            value /= 1024.0
            power += 1
        return (value, power)

    @classmethod
    def _human_readable(cls, value: float, suffix: str = 'B') -> str:
        value, power = cls._units(value)
        return f"{value:6.1f} {cls.PREFIXES[power]}{suffix}"

    def _text_current(self) -> str:
        return self._human_readable(float(self.current_bytes))

    def _text_total(self) -> str:
        return self._human_readable(float(self.total_bytes))

    def _should_update(self, progress: float) -> bool:
        diff = abs(self._units(progress)[0] - self._units(self.progress)[0])
        return super()._should_update(progress) or diff > 0.1

    def update_by_blocks_and_print(self, block_count: int, block_size: int, total_size: int) -> None:
        if total_size <= 0:
            return
        self.total_bytes = total_size
        size = block_count * block_size
        progress = size / total_size

        if self._should_update(progress):
            self.current_bytes = size
            self.print()


def _num_text_len(n: int) -> int:
    return len(str(n))


@dataclass
class CountBar(ProgressBar):
    current_count: int = 0
    total_count: int = 0

    @property
    def progress(self) -> float:
        return self.current_count / self.total_count

    def _text_current(self) -> str:
        return f"{' ' * (_num_text_len(self.total_count) - _num_text_len(self.current_count))}{self.current_count}"

    def _text_total(self) -> str:
        return f"{self.total_count}"

    def update_and_print(self, count: int) -> None:
        if count != self.current_count:
            self.current_count = count
            self.print()
