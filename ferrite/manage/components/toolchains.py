from __future__ import annotations
import os
import shutil
import tarfile
from urllib.request import urlretrieve
import logging
from manage.components.base import Component
from manage.tasks.base import Task, TaskArgs
from manage.utils.run import run
from manage.paths import target_dir

class ToolchainDownloadHook:
    def _format_bytes(self, num, suffix='B'):
        for unit in ['','Ki','Mi','Gi','Ti','Pi','Ei','Zi']:
            if abs(num) < 1024.0:
                return f"{num:4.1f} {unit}B"
            num /= 1024.0
        return f"{num:.1f} YiB"

    def _progress_bar(self, progress):
        part = int(self.bar_length * progress)
        return "{}{}".format(
            self.bar_chars[1] * part,
            self.bar_chars[0] * (self.bar_length - part),
        )

    def __init__(self, bar_length=32, bar_chars=".#"):
        self.bar_length = bar_length
        self.bar_chars = bar_chars
        print("[{}] {}".format(
            self._progress_bar(0.0),
            self._format_bytes(0),
        ), end="")

    def __call__(self, block_count, block_size, total_size):
        if total_size <= 0:
            return
        
        size = block_count * block_size
        progress = size / total_size
        print("\r[{}] {} of {}".format(
            self._progress_bar(progress),
            self._format_bytes(size),
            self._format_bytes(total_size),
        ), end="")

class ToolchainDownloadTask(Task):
    def __init__(self, owner):
        super().__init__()
        self.owner = owner

    def run(self, args: TaskArgs):
        self.owner.download()

class Toolchain(Component):
    def __init__(self, dir_name: str, archive: str, url: str):
        super().__init__()

        self.path = os.path.join(target_dir, dir_name)

        if "{}" in archive:
            archive = archive.format(dir_name)
        if "{}" in url:
            url = url.format(archive)
        
        self.dir_name = dir_name
        self.archive = archive
        self.url = url

        self.download_task = ToolchainDownloadTask(self)

    def download(self):
        if os.path.exists(self.path):
            return

        tmp_dir = os.path.join(target_dir, "download")
        os.makedirs(tmp_dir, exist_ok=True)

        archive_path = os.path.join(tmp_dir, self.archive)
        if not os.path.exists(archive_path):
            try:
                logging.info(f"Loading toolchain {self.archive} ...")
                urlretrieve(self.url, archive_path, ToolchainDownloadHook())
                print()
            except:
                if os.path.exists(archive_path):
                    os.remove(archive_path)
                raise

        logging.info(f"Extracting toolchain {self.archive} ...")
        dir_path = os.path.join(tmp_dir, self.dir_name)
        try:
            shutil.unpack_archive(archive_path, tmp_dir)
        except:
            if os.path.exists(dir_path):
                shutil.rmtree(dir_path)
            raise

        shutil.move(dir_path, self.path)
    
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
