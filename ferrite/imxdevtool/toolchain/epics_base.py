import os

from imxdevtool.toolchain import ToolLoader
from imxdevtool.util.subproc import run, SubprocError
from imxdevtool.util.files import substitute


def clone(path):
        run(
            ["git", "clone", "https://github.com/epics-base/epics-base.git", os.path.basename(path)],
            cwd=os.path.dirname(path),
        )
        run(
            ["git", "checkout", "R7.0.3.1"],
            cwd=path,
        )
        run(
            ["git", "submodule", "update", "--init", "--recursive"],
            cwd=path,
        )

def configure_toolchain(epicsdir, toolchain):
    if toolchain is None:
        substitute([
            (r'^([ \t]*CROSS_COMPILER_TARGET_ARCHS[ \t]*=[ \t]*)[^\n]*$', r'\1'),
        ], os.path.join(epicsdir, "configure/CONFIG_SITE"))
        
    else:
        substitute([
            (r'^([ \t]*CROSS_COMPILER_TARGET_ARCHS[ \t]*=[ \t]*)[^\n]*$', r'\1linux-arm'),
        ], os.path.join(epicsdir, "configure/CONFIG_SITE"))

        substitute([
            (r'^([ \t]*GNU_TARGET[ \t]*=[ \t]*)[^\n]*$', r'\1arm-linux-gnueabihf'),
            (r'^([ \t]*GNU_DIR[ \t]*=[ \t]*)[^\n]*$', r'\1{}'.format(toolchain)),
        ], os.path.join(epicsdir, "configure/os/CONFIG_SITE.linux-x86.linux-arm"))

def configure_outdir(epicsdir, outdir):
    if outdir is None:
        substitute([
            (r'^[ \t]*#*([ \t]*INSTALL_LOCATION[ \t]*=[ \t]*)[^\n]*$', r'#\1'),
        ], os.path.join(epicsdir, "configure/CONFIG_SITE"))

    else:
        substitute([
            (r'^[ \t]*#*([ \t]*INSTALL_LOCATION[ \t]*=[ \t]*)[^\n]*$', r'\1{}'.format(outdir)),
        ], os.path.join(epicsdir, "configure/CONFIG_SITE"))

def build(epicsdir):
    run(["make"], cwd=epicsdir)

def clean(epicsdir):
    try:
        run(["make", "clean", "uninstall"], cwd=epicsdir)
    except SubprocError:
        run(["git", "clean", "-dfX"], cwd=epicsdir)


class EpicsLoader(ToolLoader):
    def __init__(self, toolchain=None, outdir=None):
        self.outdir = outdir
        self.toolchain = toolchain

    def load(self, path):
        if not os.path.exists(path):
            clone(path)
        else:
            clean(path)

        configure_toolchain(path, self.toolchain)
        configure_outdir(path, None)
        build(path)
        
        if self.outdir is not None:
            configure_outdir(path, self.outdir)
            build(path)
