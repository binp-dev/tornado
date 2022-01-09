import os
from subprocess import Popen
import time


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
