import os
from subprocess import run

from script.ioctool.build import build


def test(**kwargs):
    outdir = kwargs["output_dir"]
    if outdir is None:
        outdir = os.path.join(kwargs["top"], "build/unittest")

    os.makedirs(outdir, exist_ok=True)
    run(["cmake", os.path.join(kwargs["top"])], cwd=outdir, check=True)
    run(["make"], cwd=outdir, check=True)
    run(["ctest", "-V"], cwd=outdir, check=True)
