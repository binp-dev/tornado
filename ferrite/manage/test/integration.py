import os
from subprocess import run, Popen
import time
import zmq

from manage.build import build
from manage import ca


class Ioc:
    def __init__(self, binary, script):
        self.binary = binary
        self.script = script
        self.proc = None

    def __enter__(self):
        self.proc = Popen(
            [self.binary, os.path.basename(self.script)],
            cwd=os.path.dirname(self.script),
            text=True
        )
        time.sleep(1)
        print("ioc '%s' started")

    def __exit__(self, *args):
        print("terminating '%s' ...")
        self.proc.terminate()
        print("ioc '%s' terminated")


def test(**kwargs):
    if kwargs["output_dir"] is None:
        kwargs["output_dir"] = os.path.join(kwargs["top"], "build/unittest")

    cflags = "-DTEST -DBACKTRACE"
    build(**kwargs, opts=[
        "USR_CFLAGS={}".format(cflags), "USR_CXXFLAGS={}".format(cflags),
        "USR_LDFLAGS=",
        "LIB_SYS_LIBS=czmq zmq"
    ])

    ioc = Ioc(
        os.path.join(kwargs["output_dir"], "bin", kwargs["host_arch"], "PSC"),
        os.path.join(kwargs["output_dir"], "iocBoot/iocPSC/st.cmd")
    )

    context = zmq.Context()
    socket = context.socket(zmq.PAIR)
    socket.bind("tcp://127.0.0.1:8321")

    prefix = os.path.join(kwargs["epics_base"], "bin", kwargs["host_arch"])
    with ca.Repeater(prefix), ioc:
        for _ in range(100):
            time.sleep(0.01)
            socket.send(b"Hello")
            pts = socket.recv()
            assert len(pts) == (256//3)*3
            assert all([0 == c for c in pts])
