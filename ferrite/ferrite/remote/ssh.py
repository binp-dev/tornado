from __future__ import annotations
from typing import List, Optional, Tuple

import time
from subprocess import Popen
from pathlib import Path, PurePosixPath

from paramiko import SSHClient
from paramiko.channel import ChannelFile

from ferrite.utils.run import run, RunError
from ferrite.utils.strings import quote
from ferrite.remote.base import Connection, Device

import logging

logger = logging.getLogger(__name__)


def _split_addr(addr: str) -> Tuple[str, int]:
    comps = addr.split(":")
    if len(comps) == 2:
        return comps[0], int(comps[1])
    elif len(comps) == 1:
        return addr, 22
    else:
        raise Exception(f"Bad address format: '{addr}'")


class _ParamikoConnection(Connection):

    def __init__(self, client: SSHClient, channels: List[ChannelFile]) -> None:
        self.client = client
        self.channels = channels

    def close(self) -> None:
        self.client.close()


class SshConnection(Connection):

    def __init__(self, proc: Popen[bytes]) -> None:
        self.proc = proc

    def close(self) -> None:
        self.proc.terminate()


class SshDevice(Device):

    def __init__(self, host: str, port: Optional[int] = None, user: str = "root"):
        super().__init__()

        if port is not None:
            self.host = host
            self.port = port
        else:
            self.host, self.port = _split_addr(host)

        self.user = user

    def store(self, src: Path, dst: PurePosixPath, recursive: bool = False) -> None:
        if not recursive:
            run(["bash", "-c", f"test -f {src} && cat {src} | ssh -p {self.port} {self.user}@{self.host} 'cat > {dst}'"])
        else:
            run(["rsync", "-rlpt", "--progress", "--rsh", f"ssh -p {self.port}", f"{src}/", f"{self.user}@{self.host}:{dst}"])

    def store_mem(self, src_data: str, dst_path: PurePosixPath) -> None:
        logger.debug(f"Store {len(src_data)} chars to {self.name()}:{dst_path}")
        logger.debug(src_data)
        run(["bash", "-c", f"echo {quote(src_data)} | ssh -p {self.port} {self.user}@{self.host} 'cat > {dst_path}'"])

    def _prefix(self) -> List[str | Path]:
        return ["ssh", "-p", str(self.port), f"{self.user}@{self.host}"]

    def name(self) -> str:
        return f"{self.user}@{self.host}:{self.port}"

    def run(self, args: List[str], wait: bool = True) -> Optional[SshConnection]:
        argstr = " ".join([quote(a) for a in args])
        if wait:
            logger.info(f"SSH run {self.name()} {args}")
            run(self._prefix() + [argstr])
            return None
        else:
            logger.info(f"SSH popen {self.name()} {args}")
            return SshConnection(Popen(self._prefix() + [argstr]))

    def _paramiko_client(self) -> SSHClient:
        client = SSHClient()
        client.load_system_host_keys()
        client.connect(self.host, port=self.port, username=self.user)
        return client

    def _paramiko_run(self, args: List[str], wait: bool = True) -> Optional[_ParamikoConnection]:
        client = self._paramiko_client()
        argstr = " ".join([quote(a) for a in args])
        logger.info(f"SSH run {self.name()} {args}")
        _, stdout, stderr = client.exec_command(argstr, timeout=None if wait else 0)
        if wait:
            print(stderr.read())
            print(stdout.read())
            return None
        else:
            logger.info(f"SSH popen {self.name()} {args}")
            return _ParamikoConnection(client, [stderr, stdout])

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

        logger.info("Waiting for device to reboot ...")
        self.wait_online()
        logger.info("Rebooted")
