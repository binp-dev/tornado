from __future__ import annotations
from typing import Dict, List

import shutil
import logging
from pathlib import Path
from dataclasses import dataclass

from ferrite.utils.run import capture
from ferrite.utils.net import download_alt
from ferrite.components.base import Artifact, Component, Task, Context


@dataclass
class Target:
    isa: str # Instruction set architecture
    api: str
    abi: str

    @staticmethod
    def from_str(triple: str) -> Target:
        return Target(*triple.split("-"))

    def __str__(self) -> str:
        return f"{self.isa}-{self.api}-{self.abi}"


@dataclass
class Toolchain(Component):
    name: str
    target: Target
    cached: bool = False


class HostToolchain(Toolchain):

    def __init__(self) -> None:
        super().__init__("host", Target.from_str(capture(["gcc", "-dumpmachine"])))

    def tasks(self) -> Dict[str, Task]:
        return {}


class CrossToolchain(Toolchain):

    class DownloadTask(Task):

        def __init__(self, owner: CrossToolchain):
            super().__init__()
            self.owner = owner

        def run(self, ctx: Context) -> None:
            self.owner.download()

        def artifacts(self) -> List[Artifact]:
            return [Artifact(self.owner.path, cached=self.owner.cached)]

    def __init__(self, name: str, target: Target, target_dir: Path, dir_name: str, archive: str, urls: List[str]):
        super().__init__(name, target, cached=True)

        self.target_dir = target_dir

        self.dir_name = dir_name
        self.archive = archive
        self.urls = urls

        self.path = target_dir / f"toolchain_{self.dir_name}"

        self.download_task = self.DownloadTask(self)

    def download(self) -> bool:
        if self.path.exists():
            logging.info(f"Toolchain {self.archive} is already downloaded")
            return False

        tmp_dir = self.target_dir / "download"
        tmp_dir.mkdir(exist_ok=True)

        archive_path = tmp_dir / self.archive
        if not archive_path.exists():
            logging.info(f"Loading toolchain {self.archive} ...")
            download_alt(self.urls, archive_path)
        else:
            logging.info(f"Toolchain archive {self.archive} already exists")

        logging.info(f"Extracting toolchain {self.archive} ...")
        dir_path = tmp_dir / self.dir_name
        try:
            shutil.unpack_archive(archive_path, tmp_dir)
        except:
            if dir_path.exists():
                shutil.rmtree(dir_path)
            raise

        shutil.move(dir_path, self.path)

        return True

    def tasks(self) -> Dict[str, Task]:
        return {
            "download": self.download_task,
        }
