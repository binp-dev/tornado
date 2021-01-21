from __future__ import annotations
import os
import shutil
import tarfile
import logging
from manage.components.base import Component, Task, Context
from manage.utils.run import run
from manage.utils.net import download
from manage.paths import TARGET_DIR

class ToolchainDownloadTask(Task):
    def __init__(self, owner):
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> bool:
        return self.owner.download()

class Toolchain(Component):
    def __init__(self, dir_name: str, archive: str, url: str):
        super().__init__()

        self.path = os.path.join(TARGET_DIR, dir_name)

        if "{}" in archive:
            archive = archive.format(dir_name)
        if "{}" in url:
            url = url.format(archive)
        
        self.dir_name = dir_name
        self.archive = archive
        self.url = url

        self.download_task = ToolchainDownloadTask(self)

    def download(self) -> bool:
        if os.path.exists(self.path):
            logging.info(f"Toolchain {self.archive} is already downloaded")
            return False

        tmp_dir = os.path.join(TARGET_DIR, "download")
        os.makedirs(tmp_dir, exist_ok=True)

        archive_path = os.path.join(tmp_dir, self.archive)
        if not os.path.exists(archive_path):
            try:
                logging.info(f"Loading toolchain {self.archive} ...")
                download(self.url, archive_path)
            except:
                if os.path.exists(archive_path):
                    os.remove(archive_path)
                raise
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

class AppToolchain(Toolchain):
    def __init__(self):
        super().__init__(
            "gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf",
            "{}.tar.xz",
            "http://releases.linaro.org/components/toolchain/binaries/7.5-2019.12/arm-linux-gnueabihf/{}",
        )

class McuToolchain(Toolchain):
    def __init__(self):
        super().__init__(
            "gcc-arm-none-eabi-5_4-2016q3",
            "{}-20160926-linux.tar.bz2",
            "https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/{}",
        )
