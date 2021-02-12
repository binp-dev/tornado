import os
from urllib.request import urlretrieve
from urllib.error import HTTPError
import logging

class DownloadHook:
    def _format_bytes(self, num, suffix='B'):
        for unit in ["  ","Ki","Mi","Gi","Ti","Pi","Ei","Zi"]:
            if abs(num) < 1024.0:
                return f"{num:6.1f} {unit}B"
            num /= 1024.0
        return f"{num:.1f} YiB"

    def _progress_bar(self, progress):
        part = int(round(self.bar_length * progress))
        return "{}{}".format(
            self.bar_chars[1] * part,
            self.bar_chars[0] * (self.bar_length - part),
        )

    def __init__(self, bar_length=64, bar_chars=".#"):
        self.bar_length = bar_length
        self.bar_chars = bar_chars
        print("[{}] {}".format(
            self._progress_bar(0.0),
            self._format_bytes(0),
        ), end="", flush=True)
        self.prev_progress = 0.0

    def __call__(self, block_count, block_size, total_size):
        if total_size <= 0:
            return
        
        size = block_count * block_size
        progress = size / total_size
        if progress > self.prev_progress + 0.5 / self.bar_length:
            print("\r[{}] {} of {}".format(
                self._progress_bar(progress),
                self._format_bytes(size),
                self._format_bytes(total_size),
            ), end="", flush=True)
            self.prev_progress = progress

def download(src_url, dst_path):
    logging.debug(f"downloading from '{src_url}' ...")
    try:
        urlretrieve(src_url, dst_path, DownloadHook())
    except:
        print(flush=True)
        if os.path.exists(dst_path):
            os.remove(dst_path)
        logging.debug(f"download failed")
        raise
    else:
        print(flush=True)
        logging.debug(f"downloaded to '{dst_path}'")

def download_alt(src_urls, dst_path):
    for url in src_urls:
        last_error = None
        try:
            download(url, dst_path)
            break
        except HTTPError as e:
            last_error = e
            logging.warning(str(e))
            continue
    if last_error is not None:
        raise last_error
