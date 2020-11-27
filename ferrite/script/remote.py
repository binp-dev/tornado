from script.util.subproc import run
from subprocess import Popen


def _split_addr(addr):
    comps = addr.split(":")
    if len(comps) == 2:
        return comps
    elif len(comps) == 1:
        return addr, "22"
    else:
        raise Exception(f"Bad address format: '{addr}'")

class Remote:
    def __init__(self, *args, user="root"):
        if len(args) == 2:
            self.host, self.port = args
        elif len(args) == 1:
            self.host, self.port = _split_addr(args[0])
        else:
            raise Exception(f"Bad Remote args format")
        self.user = user

    def store(self, src, dst, r=False):
        if not r:
            run([
                "bash", "-c",
                f"test -f {src} && cat {src} | ssh -p {self.port} {self.user}@{self.host} 'cat > {dst}'"
            ])
        else:
            run([
                "rsync", "-lr",
                "--rsh", f"ssh -p {self.port}",
                src,
                f"{self.user}@{self.host}:{dst}",
            ])

    def _prefix(self):
        return ["ssh", "-p", self.port, f"{self.user}@{self.host}"]

    def run(self, args):
        run(self._prefix() + args)

    def popen(self, args):
        return Popen(self._prefix() + args)
