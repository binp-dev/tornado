import subprocess
import logging as log


SubprocError = subprocess.CalledProcessError

def run(proc_args, **kwargs):
    log.info("running: " + " ".join(proc_args))
    return subprocess.run(proc_args, check=True, **kwargs)
