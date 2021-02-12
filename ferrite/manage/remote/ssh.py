import time
import logging
from subprocess import Popen
from utils.run import run, RunError
from utils.strings import quote
from .base import Device

def _split_addr(addr):
    comps = addr.split(":")
    if len(comps) == 2:
        return comps
    elif len(comps) == 1:
        return addr, "22"
    else:
        raise Exception(f"Bad address format: '{addr}'")

class SshDevice(Device):
    def __init__(self, *args, user="root"):
        super().__init__()

        if len(args) == 2:
            self.host, self.port = args
        elif len(args) == 1:
            self.host, self.port = _split_addr(args[0])
        else:
            raise Exception(f"Bad args format")

        self.user = user

    def store(self, src, dst, r=False):
        if not r:
            run([
                "bash", "-c",
                f"test -f {src} && cat {src} | ssh -p {self.port} {self.user}@{self.host} 'cat > {dst}'"
            ])
        else:
            run([
                "rsync", "-rlpt", "--progress",
                "--rsh", f"ssh -p {self.port}",
                src + "/",
                f"{self.user}@{self.host}:{dst}",
            ])

    def store_mem(self, src_data, dst_path):
        logging.debug(f"Store {len(src_data)} chars to {self.name()}:{dst_path}")
        logging.debug(src_data)
        run([
            "bash", "-c",
            f"echo {quote(src_data)} | ssh -p {self.port} {self.user}@{self.host} 'cat > {dst_path}'"
        ], log=False)

    def _prefix(self):
        return ["ssh", "-p", self.port, f"{self.user}@{self.host}"]

    def name(self):
        return f"{self.user}@{self.host}:{self.port}";

    def run(self, args, popen=False):
        argstr = " ".join([quote(a) for a in args])
        if not popen:
            logging.info(f"SSH run {self.name()} {args}")
            run(self._prefix() + [argstr], log=False)
        else:
            logging.info(f"SSH popen {self.name()} {args}")
            return Popen(self._prefix() + [argstr])

    def wait_online(self, attempts=10, timeout=10.0):
        time.sleep(timeout)
        for i in range(attempts - 1, -1, -1):
            try:
                self.run(["uname", "-a"])
            except RunError:
                if i > 0:
                    time.sleep(timeout)
                    continue
                else:
                    raise
            else:
                conn = True
                break

    def reboot(self):
        try:
            self.run(["reboot", "now"])
        except:
            pass

        logging.info("Waiting for device to reboot ...")
        self.wait_online()
        logging.info("Rebooted")
