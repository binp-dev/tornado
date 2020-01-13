import os
from subprocess import run

from tools.files import substitute


def load(dst):
    run(["git", "clone", "https://github.com/epics-base/epics-base.git", os.path.basename(dst)],
        cwd=os.path.dirname(dst), check=True,
    )
    run(["git", "checkout", "R7.0.3.1"],
        cwd=dst, check=True,
    )
    run(["git", "submodule", "update", "--init", "--recursive"],
        cwd=dst, check=True,
    )

def armconf(dst, toolchain):
    substitute([
        (r'^(\s*CROSS_COMPILER_TARGET_ARCHS\s*=\s*).*$', r'\1linux-arm'),
    ], os.path.join(dst, "configure/CONFIG_SITE"))

    substitute([
        (r'^(\s*GNU_TARGET\s*=\s*).*$', r'\1arm-linux-gnueabihf'),
        (r'^(\s*GNU_DIR\s*=\s*).*$', r'\1"{}"'.format(toolchain)),
    ], os.path.join(dst, "configure/os/CONFIG_SITE.linux-x86.linux-arm"))

def build(dst):
    pass
