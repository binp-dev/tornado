import os
from subprocess import run

from manage.test import unit, integration


def test(tests="all", **kwargs):
    if tests == "all" or tests == "unit":
        unit.test(**kwargs)
    if tests == "all" or tests == "integration":
        integration.test(**kwargs)
