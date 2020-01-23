import os
from subprocess import run

from manage.build import build


def test(**kwargs):
    build(**kwargs, opts=["USR_CFLAGS=-DTEST", "USR_CXXFLAGS=-DTEST"])
    print("integration test")
