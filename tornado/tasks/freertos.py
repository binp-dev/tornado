from __future__ import annotations

from vortex.utils.path import TargetPath
from vortex.tasks.git import RepoSource, RepoList


class Freertos(RepoList):
    def __init__(self) -> None:
        branch = "mcuxpresso_sdk_2.10.x-var01"
        super().__init__(
            TargetPath(branch),
            [
                RepoSource("https://gitlab.inp.nsk.su/psc/freertos-variscite.git", branch),
                RepoSource("https://github.com/varigit/freertos-variscite.git", branch),
            ],
        )
