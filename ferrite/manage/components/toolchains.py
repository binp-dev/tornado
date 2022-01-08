from __future__ import annotations
import os
import shutil
import tarfile
import logging
from dataclasses import dataclass
from utils.run import run
from utils.net import download_alt
from utils.strings import try_format
from manage.components.base import Component, Task, Context
from manage.paths import TARGET_DIR

@dataclass
class Target:
    isa: str # Instruction set architecture
    api: str
    abi: str

    @staticmethod
    def from_str(triple: str) -> Target:
        return Target(*triple.split("-"))

    def __str__(self):
        return f"{self.isa}-{self.api}-{self.abi}"

class Toolchain(Component):
    def __init__(self, name: str, target: Target):
        super().__init__()

        self.name = name
        self.target = target

class HostToolchain(Toolchain):
    def __init__(self):
        super().__init__("host", Target.from_str(run(["gcc", "-dumpmachine"], capture=True)))

    def tasks(self) -> dict[str, Task]:
        return {}

class ToolchainDownloadTask(Task):
    def __init__(self, owner: CrossToolchain):
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> bool:
        return self.owner.download()
    
    def artifacts(self) -> list[str]:
        return [self.owner.path]

class CrossToolchain(Toolchain):
    def __init__(self, name, target, dir_name, archive, urls):
        super().__init__(name, target)
        info = {"target": str(self.target)}

        self.dir_name = try_format(dir_name, **info)
        info["dir_name"] = self.dir_name

        self.archive = try_format(archive, **info)
        info["archive"] = self.archive

        self.urls = [try_format(url, **info) for url in urls]

        self.path = os.path.join(TARGET_DIR, f"toolchain_{self.dir_name}")

        self.download_task = ToolchainDownloadTask(self)

    def download(self) -> bool:
        if os.path.exists(self.path):
            logging.info(f"Toolchain {self.archive} is already downloaded")
            return False

        tmp_dir = os.path.join(TARGET_DIR, "download")
        os.makedirs(tmp_dir, exist_ok=True)

        archive_path = os.path.join(tmp_dir, self.archive)
        if not os.path.exists(archive_path):
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
    
    def tasks(self) -> dict[str, Task]:
        return {
            "download": self.download_task,
        }

class AppToolchain(CrossToolchain):
    def __init__(self, name, target):
        super().__init__(
            name=name,
            target=target,
            dir_name="gcc-linaro-7.5.0-2019.12-x86_64_{target}",
            archive="{dir_name}.tar.xz",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/{archive}",
                "http://releases.linaro.org/components/toolchain/binaries/7.5-2019.12/{target}/{archive}",
            ],
        )

class AppToolchainImx7(AppToolchain):
    def __init__(self):
        super().__init__(
            name="imx7",
            target=Target("arm", "linux", "gnueabihf"),
        )

class AppToolchainImx8mn(AppToolchain):
    def __init__(self):
        super().__init__(
            name="imx8mn",
            target=Target("aarch64", "linux", "gnu"),
        )

class McuToolchainImx7(CrossToolchain):
    def __init__(self):
        super().__init__(
            name="imx7",
            target=Target("arm", "none", "eabi"),
            dir_name="gcc-{target}-5_4-2016q3",
            archive="{dir_name}-20160926-linux.tar.bz2",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/{archive}",
                "https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/{archive}",
            ],
        )

class McuToolchainImx8mn(CrossToolchain):
    def __init__(self):
        super().__init__(
            name="imx8mn",
            target=Target("arm", "none", "eabi"),
            dir_name="gcc-{target}-9-2020-q2-update",
            archive="{dir_name}-x86_64-linux.tar.bz2",
            urls=[
                "https://gitlab.inp.nsk.su/psc/storage/-/raw/master/toolchains/{archive}",
                "https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/{archive}",
            ],
        )
