from __future__ import annotations
from typing import List, Optional

from pathlib import Path, PurePosixPath
from subprocess import Popen


class Device:

    def name(self) -> str:
        raise NotImplementedError()

    def store(self, local_path: Path, remote_path: PurePosixPath, recursive: bool = False) -> None:
        raise NotImplementedError()

    def store_mem(self, src_data: str, dst_path: PurePosixPath) -> None:
        raise NotImplementedError()

    def run(self, args: List[str], popen: bool = False) -> Optional[Popen[bytes]]:
        raise NotImplementedError()

    def reboot(self) -> None:
        raise NotImplementedError()
