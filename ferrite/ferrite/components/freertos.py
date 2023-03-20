from __future__ import annotations

from pathlib import Path

from ferrite.components.git import RepoSource, RepoList


class Freertos(RepoList):

    def __init__(self, dst_dir: Path, branch: str) -> None:
        super().__init__(
            dst_dir,
            [
                RepoSource("https://gitlab.inp.nsk.su/psc/freertos-variscite.git", branch),
                RepoSource("https://github.com/varigit/freertos-variscite.git", branch),
            ],
            cached=True,
        )


class FreertosImx7(Freertos):

    def __init__(self, target_dir: Path) -> None:
        branch = "freertos_bsp_1.0.1_imx7d-var01"
        super().__init__(target_dir / branch, branch)


class FreertosImx8mn(Freertos):

    def __init__(self, target_dir: Path) -> None:
        branch = "mcuxpresso_sdk_2.9.x-var01"
        super().__init__(target_dir / branch, branch)
