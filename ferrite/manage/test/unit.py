import os
from subprocess import run


def test(top, epics_base, output_dir=None, **kwargs):
    if output_dir is None:
        output_dir = top
    
    for path in os.listdir(top):
        fullpath = os.path.join(top, path)

        if path.endswith("Sup") and os.path.isdir(fullpath):
            name = path[:-3]
            unittest_dir = os.path.join(fullpath, "unittest")

            if os.path.isdir(unittest_dir):
                run(
                    [
                        "make", "test",
                        "TOP={}".format(top),
                        "EPICS_BASE={}".format(epics_base),
                        "INSTALL_LOCATION={}".format(output_dir),
                        "NAME={}".format(name)
                    ],
                    cwd=unittest_dir,
                    check=True,
                )
