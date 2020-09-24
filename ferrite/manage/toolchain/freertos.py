import os

from manage.toolchain import ToolLoader
from manage.util.subproc import run

class FreertosLoader(ToolLoader):
    def __init__(self):
        pass

    def load(self, dstdir):
        if os.path.exists(dstdir):
            return
        
        run(
            [
                "git", "clone", "https://github.com/varigit/freertos-variscite.git",
                "-b", "freertos_bsp_1.0.1_imx7d-var01", os.path.basename(dstdir),
            ],
            cwd=os.path.dirname(dstdir),
        )

loader = FreertosLoader()
