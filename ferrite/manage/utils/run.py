import logging
import subprocess

RunError = subprocess.CalledProcessError

def run(cmd, **kwargs):
    logging.debug(f"run({cmd}, {kwargs})")
    subprocess.run(cmd, check=True, **kwargs)
