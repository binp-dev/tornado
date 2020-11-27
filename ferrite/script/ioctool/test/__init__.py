import os
from subprocess import run

from script.ioctool.test import unittest, integration


def test(tests="all", **kwargs):
    if tests == "all" or tests == "unittest":
        unittest.test(**kwargs)
    if tests == "all" or tests == "integration":
        kwargs["host_arch"] = run(
            [os.path.join(kwargs["epics_base"], "startup/EpicsHostArch")],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
        integration.test(**kwargs)
