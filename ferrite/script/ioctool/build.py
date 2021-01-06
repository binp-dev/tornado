import sys, os
from subprocess import run


def build(
    top, epics_base,
    target=None, output_dir=None,
    clean=False,
    threads=None,
    cflags=None,
    ldflags=None,
    libs=None,
    opts=None,
    **kwargs,
):
    args = ["make", "clean"]
    base_flags = ["-std=c++17", "-fno-exceptions"]

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
    if cflags is not None:
        flagsstr = " ".join(base_flags + cflags)
        args += ["USR_CFLAGS={}".format(flagsstr), "USR_CXXFLAGS={}".format(flagsstr)]
    if ldflags is not None:
        args.append("USR_LDFLAGS={}".format(" ".join(ldflags)))
    if libs is not None:
        args.append("LIB_SYS_LIBS={}".format(" ".join(libs)))
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
