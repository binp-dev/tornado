import os
import logging as log

from script.util.subproc import run
from script.remote import Remote
from script import Component


class Mcu(Component):
    def setup(self, args, tools):
        self.path = os.path.join(args["top"], "mcu")
        self.output = os.path.join(args["output_dir"], "mcu")
        self.tools = tools
        self.device = args.get("dev_addr", None)

    def build(self):
        run(["mkdir", "-p", self.output])
        run(
            [
                "cmake",
                "-DCMAKE_TOOLCHAIN_FILE={}".format(os.path.join(
                    self.tools["freertos"].path,
                    "tools/cmake_toolchain_files/armgcc.cmake",
                )),
                "-G", "Unix Makefiles",
                "-DCMAKE_BUILD_TYPE=Release",
                self.path,
            ],
            add_env={
                "FREERTOS_DIR": self.tools["freertos"].path,
                "ARMGCC_DIR": self.tools["armgcc_mcu"].path,
            },
            cwd=self.output,
        )
        run(["make", "-j4"], cwd=self.output)
    
    def clean(self):
        run(["rm", "-rf", self.output])

    def deploy(self):
        self.build()
        if self.device is not None:
            remote = Remote(self.device)
            remote.store(os.path.join(self.output, "release/m4image.bin"), "/m4image.bin")
            remote.run(["mount /dev/mmcblk2p1 /mnt && mv /m4image.bin /mnt && umount /mnt"])

    def test(self):
        self.deploy()
