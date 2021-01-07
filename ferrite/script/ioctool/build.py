import sys, os
from subprocess import run


def build(
    top, epics_base,
    target=None, output_dir=None,
    clean=False,
    threads=None,
    cflags=[],
    cxxflags=[],
    ldflags=[],
    libs=[],
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

    if cflags:
        fstr = " ".join(cflags)
        args.append("USR_CFLAGS={}".format(fstr))

    if cxxflags:
        fstr = " ".join(cxxflags)
        args.append("USR_CXXFLAGS={}".format(fstr))

    if ldflags:
        args.append("USR_LDFLAGS={}".format(" ".join(ldflags)))

    if libs:
        args.append("LIB_SYS_LIBS={}".format(" ".join(libs)))

    if opts is not None:
        args += opts
    
    print(args)
    run(args, cwd=top, check=True)

    if output_dir is not None:
        if not clean:
            run(["cp", "-r", "iocBoot", output_dir], cwd=top, check=True)
        else:
            run(["rm", "-r", output_dir], cwd=top)


def clean(**kwargs):
    kwargs["clean"] = True
    build(**kwargs)
