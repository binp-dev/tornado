import os
from subprocess import run

from manage.test import unit, integration


def test(tests="all", **kwargs):
    kwargs["host_arch"] = run(
        [os.path.join(kwargs["epics_base"], "startup/EpicsHostArch")],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()

    if tests == "all" or tests == "unit":
        unit.test(**kwargs)
    if tests == "all" or tests == "integration":
        integration.test(**kwargs)
