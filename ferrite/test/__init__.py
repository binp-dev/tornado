import os
from subprocess import run


def test(cwd, **kwargs):
    run(
        [
            "make", "test",
            "EPICS_BASE={}".format(kwargs["epics_base"]),
            "TOP={}".format(cwd)
        ],
        cwd=os.path.join(cwd, "PSCSup/unittest"),
        check=True,
    )
