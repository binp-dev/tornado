import os

from manage.components import ComponentLoader
from manage.tools.subproc import run


class ToolchainLoader(ComponentLoader):
    def __init__(self, name, archive, url):
        if "{}" in archive:
            archive = archive.format(name)
        if "{}" in url:
            url = url.format(archive)
        
        self.name = name
        self.archive = archive
        self.url = url

    def load(self, outdir):
        if os.path.exists(outdir):
            return

        root, name = os.path.split(outdir)
        run(["mkdir", "-p", root])

        if not os.path.exists(os.path.join(root, self.archive)):
            try:
                run(["wget", self.url], cwd=root)
            except:
                run(["rm", self.archive], cwd=root)
                raise

        try:
            run(["tar", "xvf", self.archive], cwd=root)
        except:
            run(["rm", "-rf", self.name], cwd=root)
            raise

        run(["mv", self.name, name], cwd=root)


linux_loader = ToolchainLoader(
    "gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf",
    "{}.tar.xz",
    "http://releases.linaro.org/components/toolchain/binaries/6.3-2017.05/arm-linux-gnueabihf/{}"
)

m4_loader = ToolchainLoader(
    "gcc-arm-none-eabi-5_4-2016q3",
    "gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2",
    "https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/{}"
)
