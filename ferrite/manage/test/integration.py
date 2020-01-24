import os
from subprocess import run

from manage.build import build


def test(**kwargs):
    build(**kwargs, opts=[
        "USR_CFLAGS=-DTEST", "USR_CXXFLAGS=-DTEST",
        "LIB_SYS_LIBS=czmq zmq"
    ])
    print("integration test")
