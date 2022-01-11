from __future__ import annotations
from typing import List, Optional, Tuple

import time
import logging
from subprocess import Popen

from ferrite.utils.run import run, RunError
from ferrite.utils.strings import quote
from ferrite.remote.base import Device


def _split_addr(addr: str) -> Tuple[str, int]:
    comps = addr.split(":")
    if len(comps) == 2:
        return comps[0], int(comps[1])
    elif len(comps) == 1:
        return addr, 22
    else:
        raise Exception(f"Bad address format: '{addr}'")


class SshDevice(Device):

    def __init__(self, host: str, port: Optional[int] = None, user: str = "root"):
        super().__init__()

        if port is not None:
            self.host = host
            self.port = port
        else:
            self.host, self.port = _split_addr(host)

        self.user = user

    def store(self, src: str, dst: str, recursive: bool = False) -> None:
        if not recursive:
            run(["bash", "-c", f"test -f {src} && cat {src} | ssh -p {self.port} {self.user}@{self.host} 'cat > {dst}'"])
        else:
            run(["rsync", "-rlpt", "--progress", "--rsh", f"ssh -p {self.port}", src + "/", f"{self.user}@{self.host}:{dst}"])

    def store_mem(self, src_data: str, dst_path: str) -> None:
        logging.debug(f"Store {len(src_data)} chars to {self.name()}:{dst_path}")
        logging.debug(src_data)
        run(["bash", "-c", f"echo {quote(src_data)} | ssh -p {self.port} {self.user}@{self.host} 'cat > {dst_path}'"])

    def _prefix(self) -> List[str]:
        return ["ssh", "-p", str(self.port), f"{self.user}@{self.host}"]

    def name(self) -> str:
        return f"{self.user}@{self.host}:{self.port}"

    def run(self, args: List[str], popen: bool = False) -> Optional[Popen[bytes]]:
        argstr = " ".join([quote(a) for a in args])
        if not popen:
            logging.info(f"SSH run {self.name()} {args}")
            run(self._prefix() + [argstr])
            return None
        else:
            logging.info(f"SSH popen {self.name()} {args}")
            return Popen(self._prefix() + [argstr])

    def wait_online(self, attempts: int = 10, timeout: float = 10.0) -> None:
        time.sleep(timeout)
        for i in range(attempts - 1, -1, -1):
            try:
                self.run(["uname", "-a"])
            except RunError:
                if i > 0:
                    time.sleep(timeout)
                    continue
                else:
                    raise
            else:
                conn = True
                break

    def reboot(self) -> None:
        try:
            self.run(["reboot", "now"])
        except:
            pass

        logging.info("Waiting for device to reboot ...")
        self.wait_online()
        logging.info("Rebooted")
