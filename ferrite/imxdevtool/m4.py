import os
import logging as log

from imxdevtool.util.subproc import run
from imxdevtool import Component


class M4(Component):
    def setup(self, args, tools):
        self.path = os.path.join(args["top"], "m4")
        self.output = os.path.join(args["output_dir"], "m4")
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
                "ARMGCC_DIR": self.tools["armgcc_m4"].path,
            },
            cwd=self.output,
        )
        run(["make", "-j4"], cwd=self.output)
    
    def clean(self):
        run(["rm", "-rf", self.output])

    def deploy(self):
        self.build()
        if self.device is not None:
            devcmd = "cat > m4image.bin && mount /dev/mmcblk0p1 /mnt && mv m4image.bin /mnt && umount /mnt"
            hostcmd = "test -f {img} && cat {img} | ssh root@{} '{}'".format(
                self.device, devcmd, img=os.path.join(self.output, "release/m4image.bin")
            )
            run(["bash", "-c", hostcmd])

    def test(self):
        self.deploy()
