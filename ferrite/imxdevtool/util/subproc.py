import os
import subprocess
import logging as log


SubprocError = subprocess.CalledProcessError

def run(proc_args, add_env=None, **kwargs):
    log.info("running: " + " ".join(proc_args))
    if add_env:
        env = dict(os.environ)
        env.update(add_env)
        log.info("additional env: {}".format(add_env))
    else:
        env = None
    return subprocess.run(proc_args, check=True, env=env, **kwargs)
