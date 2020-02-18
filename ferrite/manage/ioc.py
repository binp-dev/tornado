import os
import importlib

from manage.util.subproc import run
from manage import Component


class Ioc(Component):
    def __init__(self):
        self.path = None
        self.output = None
        self.tools = None
    
    def setup(self, args, tools):
        self.path = os.path.join(args["top"], "ioc")
        self.output = os.path.join(args["output_dir"], "ioc")
        self.tools = tools

    def manage(self, cmd):
        run(
            [
                "python3", "-m", "manage",
                "--top", self.path,
                "--output", self.output,
                "--epics-base", self.tools["epics_base"].path,
            ] + cmd,
            cwd=self.path
        )

    def build(self):
        self.manage(["build"])
    
    def clean(self):
        self.manage(["clean"])

    def deploy(self):
        self.manage(["build", "--target", "linux-arm"])

    def test(self):
        self.manage(["test"])
