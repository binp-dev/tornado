from __future__ import annotations
import os
import shutil
import tarfile
import logging
from utils.run import run
from utils.net import download
from manage.components.base import Component, Task, Context
from manage.paths import TARGET_DIR

class ToolchainDownloadTask(Task):
    def __init__(self, owner):
        super().__init__()
        self.owner = owner

    def run(self, ctx: Context) -> bool:
        return self.owner.download()

class Toolchain(Component):
    def __init__(self, target, dir_name, archive, url):
        super().__init__()

        raw_info = [
            ("target", target),
            ("dir_name", dir_name),
            ("archive", archive),
            ("url", url),
        ]
        info = {}
        for ks, src in raw_info:
            rep = {}
            for k, v in info.items():
                if ("{" + k + "}") in src:
                    rep[k] = v
            info[ks] = src.format(**rep)

        self.target = info["target"]
        self.dir_name = info["dir_name"]
        self.archive = info["archive"]
        self.url = info["url"]

        self.path = os.path.join(TARGET_DIR, self.dir_name)

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
            target="arm-linux-gnueabihf",
            dir_name="gcc-linaro-7.5.0-2019.12-x86_64_{target}",
            archive="{dir_name}.tar.xz",
            url="http://releases.linaro.org/components/toolchain/binaries/7.5-2019.12/{dir_name}/{archive}",
        )

class McuToolchain(Toolchain):
    def __init__(self):
        super().__init__(
            target="arm-none-eabi",
            dir_name="gcc-{target}-5_4-2016q3",
            archive="{dir_name}-20160926-linux.tar.bz2",
            url="https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/{archive}",
        )
