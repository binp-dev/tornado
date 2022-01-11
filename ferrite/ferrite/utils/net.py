from __future__ import annotations
from typing import List

import os
from pathlib import Path
from urllib.request import urlretrieve
from urllib.error import HTTPError, URLError
import logging


class DownloadHook:

    def _format_bytes(self, num: float, suffix: str = 'B') -> str:
        for unit in ["  ", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"]:
            if abs(num) < 1024.0:
                return f"{num:6.1f} {unit}B"
            num /= 1024.0
        return f"{num:.1f} YiB"

    def _progress_bar(self, progress: float) -> str:
        part = int(round(self.bar_length * progress))
        return "{}{}".format(
            self.bar_chars[1] * part,
            self.bar_chars[0] * (self.bar_length - part),
        )

    def __init__(self, bar_length: int = 64, bar_chars: str = ".#"):
        self.bar_length = bar_length
        self.bar_chars = bar_chars
        print(
            "[{}] {}".format(
                self._progress_bar(0.0),
                self._format_bytes(0),
            ), end="", flush=True
        )
        self.prev_progress = 0.0

    def __call__(self, block_count: int, block_size: int, total_size: int) -> None:
        if total_size <= 0:
            return

        size = block_count * block_size
        progress = size / total_size
        if progress > self.prev_progress + 0.5 / self.bar_length:
            print(
                "\r[{}] {} of {}".format(
                    self._progress_bar(progress),
                    self._format_bytes(size),
                    self._format_bytes(total_size),
                ),
                end="",
                flush=True
            )
            self.prev_progress = progress


def download(src_url: str, dst_path: Path) -> None:
    logging.debug(f"downloading from '{src_url}' ...")
    try:
        urlretrieve(src_url, dst_path, DownloadHook())
    except:
        print(flush=True)
        dst_path.unlink(missing_ok=True)
        logging.warning(f"download failed")
        raise
    else:
        print(flush=True)
        logging.debug(f"downloaded to '{dst_path}'")


def download_alt(src_urls: List[str], dst_path: Path) -> None:
    for url in src_urls:
        last_error = None
        try:
            download(url, dst_path)
            break
        except (HTTPError, URLError) as e:
            last_error = e
            logging.warning(str(e))
            continue
    if last_error is not None:
        raise last_error
