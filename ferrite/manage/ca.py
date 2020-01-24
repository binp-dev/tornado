import os
from subprocess import Popen, run, PIPE
import time


def _try_join(prefix, name):
    if prefix is None:
        return name
    else:
        return os.path.join(prefix, name)

def get(pv, epics_base=None):
    print("caget %s ..." % pv)
    ret = run(
        [_try_join(prefix, "caget"), "-t", pv],
        stdout=PIPE,
        text=True,
        check=True
    )
    out = ret.stdout.strip()
    print("  %s" % out)
    return out

def put(pv, value, prefix=None):
    print("caput %s %s ..." % (pv, str(value)))
    ret = run(
        [_try_join(prefix, "caput"), "-t", pv, str(value)],
        stdout=PIPE,
        text=True,
        check=True
    )
    print("  done")

class Repeater:
    def __init__(self, prefix=None):
        self.proc = None
        self.prefix = prefix

    def __enter__(self):
        self.proc = Popen(
            [_try_join(self.prefix, "caRepeater")],
            text=True
        )
        time.sleep(1)
        print("caRepeater started")

    def __exit__(self, *args):
        print("terminating caRepeater ...")
        self.proc.terminate()
        print("caRepeater terminated")
