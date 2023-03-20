from __future__ import annotations
from typing import List, Tuple

from pathlib import Path
from urllib.request import urlretrieve
from urllib.error import HTTPError, URLError

from ferrite.utils.progress import DownloadBar

import logging

logger = logging.getLogger(__name__)


def download(src_url: str, dst_path: Path) -> None:
    logger.debug(f"downloading from '{src_url}' ...")
    bar = DownloadBar()
    bar.print()
    try:
        urlretrieve(src_url, dst_path, bar.update_by_blocks_and_print)
    except RuntimeError:
        print(flush=True)
        dst_path.unlink(missing_ok=True)
        logger.warning(f"download failed")
        raise
    bar.current_bytes = bar.total_bytes
    bar.print()
    print(flush=True)
    logger.debug(f"downloaded to '{dst_path}'")


def download_alt(src_urls: List[str], dst_path: Path) -> None:
    for url in src_urls:
        last_error = None
        try:
            download(url, dst_path)
            break
        except (HTTPError, URLError) as e:
            last_error = e
            logger.warning(str(e))
            continue
    if last_error is not None:
        raise last_error
