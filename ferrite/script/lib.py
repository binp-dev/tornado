import os
import logging as log

from script.util.subproc import run
from script import Component


class Zmq(Component):
    def setup(self, args, tools):
        self.path = os.path.join(args["top"], f"lib/zmq")
        self.bdir = os.path.join(args["output_dir"], "tmp")
        self.idir = os.path.join(args["output_dir"], "lib")
        self.tools = tools

    def _build_for(self, arch, prefix=None):
        bdir = os.path.join(self.bdir, f"{arch}/zmq")
        idir = os.path.join(self.idir, f"{arch}/zmq")

        run(["mkdir", "-p", bdir])
        
        args = []
        if prefix is not None:
            args += [
                f"-DCMAKE_C_COMPILER={prefix}gcc",
                f"-DCMAKE_CXX_COMPILER={prefix}g++",
                f"-DCMAKE_AR={prefix}ar",
                f"-DCMAKE_LINKER={prefix}ld",
            ]

        run(
            [
                "cmake",
                "-G", "Unix Makefiles",
                *args,
                "-DCMAKE_BUILD_TYPE=Release",
                f"-DCMAKE_INSTALL_PREFIX={idir}",
                self.path,
            ],
            cwd=bdir,
        )
        run(["make", "-j4", "install"], cwd=bdir)

    def build(self):
        self._build_for("host")
        self._build_for(
            "arm-linux-gnueabihf",
            os.path.join(self.tools["armgcc_app"].path, "bin/arm-linux-gnueabihf-")
        )
    
    def clean(self):
        run(["rm", "-rf", os.path.join(self.libdir, "host/zmq")])
        run(["rm", "-rf", os.path.join(self.libdir, "arm-linux-gnueabihf/zmq")])

    def deploy(self):
        self.build()
        log.warning("Zmq library probably needs to be copied to the device too.")

    def test(self):
        pass
