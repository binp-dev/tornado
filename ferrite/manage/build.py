import sys, os
from subprocess import run


def build(
    top, epics_base,
    target=None, output_dir=None,
    clean=False,
    threads=None,
    opts=None,
    **kwargs,
):
    args = ["make", "clean"]

    if not clean:
        args += ["install"]
    else:
        args += ["uninstall"]

    if threads is not None:
        args += ["-j{}".format(threads)]

    args += ["EPICS_BASE={}".format(epics_base)]
    if target is not None:
        args += ["CROSS_COMPILER_TARGET_ARCHS={}".format(target)]
    if output_dir is not None:
        args += ["INSTALL_LOCATION={}".format(output_dir)]
    if opts is not None:
        args += opts
    
    run(args, cwd=top, check=True)

    if output_dir is not None:
        if not clean:
            run(["cp", "-r", "iocBoot", output_dir], cwd=top, check=True)
        else:
            run(["rm", "-r", output_dir], cwd=top)


def clean(**kwargs):
    kwargs["clean"] = True
    build(**kwargs)
