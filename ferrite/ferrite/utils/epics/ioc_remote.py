from __future__ import annotations
from typing import Any, Optional

import time
from pathlib import PurePosixPath

from ferrite.remote.base import Device, Connection

import logging

logger = logging.getLogger(__name__)


class IocRemoteRunner:

    def __init__(
        self,
        device: Device,
        deploy_path: PurePosixPath,
        epics_deploy_path: PurePosixPath,
        arch: str,
    ):
        super().__init__()
        self.device = device
        self.deploy_path = deploy_path
        self.epics_deploy_path = epics_deploy_path
        self.arch = arch
        self.proc: Optional[Connection] = None

    def __enter__(self) -> None:
        lib_dirs = [
            self.epics_deploy_path / "lib" / self.arch,
            self.deploy_path / "lib" / self.arch,
        ]
        self.proc = self.device.run(
            [
                "bash",
                "-c",
                "export {}; export {}; cd {} && {} {}".format(
                    f"TOP={self.deploy_path}",
                    "LD_LIBRARY_PATH={}".format(":".join(map(str, lib_dirs))),
                    f"{self.deploy_path}/iocBoot/iocPSC",
                    f"{self.deploy_path}/bin/{self.arch}/PSC",
                    "st.cmd",
                ),
            ],
            wait=False,
        )
        assert self.proc is not None
        time.sleep(1)
        logger.info("IOC started")

    def __exit__(self, *args: Any) -> None:
        logger.info("terminating IOC ...")
        assert self.proc is not None
        self.proc.close()
        logger.info("IOC terminated")
