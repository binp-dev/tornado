import os
import subprocess
import logging as log


SubprocError = subprocess.CalledProcessError

def run(proc_args, add_env=None, **kwargs):
    log.info("running: " + " ".join(proc_args))
    env = dict(os.environ)
    if add_env:
        env.update(add_env)
        log.info("additional env: {}".format(add_env))
    return subprocess.run(proc_args, check=True, env=env, **kwargs)
