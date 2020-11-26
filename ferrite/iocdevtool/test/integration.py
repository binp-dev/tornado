import os
import time
import zmq

from iocdevtool.build import build
from iocdevtool.epics import Ioc, ca
from iocdevtool.test import proto

def decode(data, ids, nbytes=3, shift=1, dlen=256):
    assert len(data) == ((dlen - shift)//nbytes)*nbytes + shift
    
    cmd = int(data[0])
    assert(cmd == ids["PSCA_WF_DATA"])

    arr = data[shift:]
    arr = [arr[i : i + nbytes] for i in range(0, len(arr), nbytes)]
    assert all([len(a) == nbytes for a in arr])
    arr = [int.from_bytes(a, "little") for a in arr]
    return (cmd, arr)

def assert_pop(queue, arr):
    print(arr)
    assert len(queue) >= len(arr)
    try:
        assert all((a == b for a, b in zip(queue, arr)))
    except AssertionError:
        print("{} != {}".format(queue[:len(arr)], arr))
        raise
    return queue[len(arr):]

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
    ids = proto.read_defines(os.path.join(kwargs["top"], "../common/proto.h"))
    def id_as_bytes(id):
        return bytes([id])

    wfs = 200
    with ca.Repeater(prefix), ioc:
        assert socket.recv() == id_as_bytes(ids["PSCA_START"])
        print("Received start signal")

        msg = "Hello from MCU!"
        socket.send(id_as_bytes(ids["PSCM_MESSAGE"]) + bytes([len(msg)]) + msg.encode("utf-8"))

        queue = [0]*wfs*2
        for _ in range(4):
            socket.send(id_as_bytes(ids["PSCM_WF_REQ"]))
            queue = assert_pop(queue, decode(socket.recv(), ids)[1])
        
        wf = list(range(wfs))
        ca.put("WAVEFORM", wf, array=True, prefix=prefix)
        queue += wf*2
        for _ in range(5):
            socket.send(id_as_bytes(ids["PSCM_WF_REQ"]))
            queue = assert_pop(queue, decode(socket.recv(), ids)[1])

        wf = [wfs - x - 1 for x in range(wfs)]
        ca.put("WAVEFORM", wf, array=True, prefix=prefix)
        queue += wf*2
        for _ in range(5):
            socket.send(id_as_bytes(ids["PSCM_WF_REQ"]))
            queue = assert_pop(queue, decode(socket.recv(), ids)[1])

        wf = [wfs + x for x in range(wfs)]
        ca.put("WAVEFORM", range(0, 2*wfs, 2), array=True, prefix=prefix)
        ca.put("WAVEFORM", wf, array=True, prefix=prefix)
        queue += wf*4
        for _ in range(9):
            socket.send(id_as_bytes(ids["PSCM_WF_REQ"]))
            queue = assert_pop(queue, decode(socket.recv(), ids)[1])

    print("Test passed!")
