from __future__ import annotations
from typing import Dict, List

import os
import shutil
import logging
from pathlib import Path
from dataclasses import dataclass

from ferrite.utils.run import capture
from ferrite.utils.net import download_alt
from ferrite.utils.strings import try_format
from ferrite.components.base import Artifact, Component, Task, Context


@dataclass
class Target:
    isa: str  # Instruction set architecture
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


class ToolchainDownloadTask(Task):

    def __init__(self, owner: CrossToolchain):
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> None:
        self.owner.download()

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.owner.path, cached=self.owner.cached)]


class CrossToolchain(Toolchain):

    def __init__(self, name: str, target: Target, target_dir: Path, dir_name: str, archive: str, urls: List[str]):
        super().__init__(name, target, cached=True)
        info = {"target": str(self.target)}

        self.target_dir = target_dir

        self.dir_name = try_format(dir_name, **info)
        info["dir_name"] = self.dir_name

        self.archive = try_format(archive, **info)
        info["archive"] = self.archive

        self.urls = [try_format(url, **info) for url in urls]

        self.path = target_dir / f"toolchain_{self.dir_name}"

        self.download_task = ToolchainDownloadTask(self)

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
        dir_path = os.path.join(tmp_dir, self.dir_name)
        try:
            shutil.unpack_archive(archive_path, tmp_dir)
        except:
            if os.path.exists(dir_path):
                shutil.rmtree(dir_path)
            raise

        shutil.move(dir_path, self.path)

        return True

    def tasks(self) -> Dict[str, Task]:
        return {
            "download": self.download_task,
        }


class AppToolchain(CrossToolchain):

    def __init__(self, name: str, target: Target, target_dir: Path):
        super().__init__(
            name=name,
            target=target,
            target_dir=target_dir,
            dir_name="gcc-linaro-7.5.0-2019.12-x86_64_{target}",
            archive="{dir_name}.tar.xz",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/{archive}",
                "http://releases.linaro.org/components/toolchain/binaries/7.5-2019.12/{target}/{archive}",
            ],
        )


class AppToolchainImx7(AppToolchain):

    def __init__(self, target_dir: Path) -> None:
        super().__init__(
            name="imx7",
            target=Target("arm", "linux", "gnueabihf"),
            target_dir=target_dir,
        )


class AppToolchainImx8mn(AppToolchain):

    def __init__(self, target_dir: Path) -> None:
        super().__init__(
            name="imx8mn",
            target=Target("aarch64", "linux", "gnu"),
            target_dir=target_dir,
        )


class McuToolchainImx7(CrossToolchain):

    def __init__(self, target_dir: Path) -> None:
        super().__init__(
            name="imx7",
            target=Target("arm", "none", "eabi"),
            target_dir=target_dir,
            dir_name="gcc-{target}-5_4-2016q3",
            archive="{dir_name}-20160926-linux.tar.bz2",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/{archive}",
                "https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/{archive}",
            ],
        )


class McuToolchainImx8mn(CrossToolchain):

    def __init__(self, target_dir: Path) -> None:
        super().__init__(
            name="imx8mn",
            target=Target("arm", "none", "eabi"),
            target_dir=target_dir,
            dir_name="gcc-{target}-9-2020-q2-update",
            archive="{dir_name}-x86_64-linux.tar.bz2",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/{archive}",
                "https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/{archive}",
            ],
        )
