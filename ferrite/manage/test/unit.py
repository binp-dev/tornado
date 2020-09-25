import os
from subprocess import run

from manage.build import build


def test(**kwargs):
    if kwargs["output_dir"] is None:
        kwargs["output_dir"] = os.path.join(kwargs["top"], "build/unittest")

    cflags = "-DUNITTEST -DTEST -DBACKTRACE"
    build(**kwargs, opts=[
        "USR_CFLAGS={}".format(cflags), "USR_CXXFLAGS={}".format(cflags),
        "LIB_SYS_LIBS=czmq zmq"
    ])

    run(
        [os.path.join(kwargs["output_dir"], "bin", kwargs["host_arch"], "PSC")],
        cwd=kwargs["output_dir"],
        check=True,
    )
