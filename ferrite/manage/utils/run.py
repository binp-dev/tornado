import os
import logging
import subprocess

RunError = subprocess.CalledProcessError

def run(cmd, add_env=None, **kwargs):
    logging.debug(f"run({cmd}, {kwargs})")
    env = dict(os.environ)
    if add_env:
        env.update(add_env)
        logging.info("additional env: {}".format(add_env))
    subprocess.run(cmd, check=True, env=env, **kwargs)
