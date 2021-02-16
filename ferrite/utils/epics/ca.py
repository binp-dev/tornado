import os
from utils.run import run
from subprocess import Popen
import time

def _env():
    return {
        "EPICS_CA_AUTO_ADDR_LIST": "NO",
        "EPICS_CA_ADDR_LIST": "127.0.0.1",
    }

def get(prefix, pv):
    print("caget %s ..." % pv)
    out = run(
        [os.path.join(prefix, "caget"), "-t", pv],
        add_env=_env(),
        capture=True,
        log=False,
    ).strip()
    print("  %s" % out)
    return out

def put(prefix, pv, value, array=False):
    print("caput %s %s ..." % (pv, str(value)))

    args = [os.path.join(prefix, "caput"), "-t"]
    if not array:
        args += [pv, str(value)]
    else:
        args += ["-a", pv, str(len(value))] + [str(v) for v in value]
    
    run(
        args,
        add_env=_env(),
        capture=True,
        log=False,
    )
    print("  done")

class Repeater:
    def __init__(self, prefix):
        self.proc = None
        self.prefix = prefix

    def __enter__(self):
        print("starting caRepeater ...")
        env = dict(os.environ)
        env.update(_env())
        self.proc = Popen(
            [os.path.join(self.prefix, "caRepeater")],
            env=env,
        )
        time.sleep(1)
        print("caRepeater started")

    def __exit__(self, *args):
        print("terminating caRepeater ...")
        self.proc.terminate()
        print("caRepeater terminated")
