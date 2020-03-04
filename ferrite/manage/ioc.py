import os
import importlib

from manage.util.subproc import run
from manage import Component


class Ioc(Component):
    def setup(self, args, tools):
        self.path = os.path.join(args["top"], "ioc")
        self.output = os.path.join(args["output_dir"], "ioc")
        self.tools = tools
        self.device = args.get("dev_addr", None)

    def manage(self, cmd, postfix):
        run(
            [
                "python3", "-m", "manage",
                "--top", self.path,
                "--output", os.path.join(self.output, postfix),
                "--epics-base", self.tools["epics_base"].path,
            ] + cmd,
            cwd=self.path
        )

    def build(self):
        self.manage(["build"], "release")
        self.manage(["build", "--target", "linux-arm"], "release")
    
    def clean(self):
        run(["rm", "-rf", self.output])

    def deploy(self):
        self.build()
        if self.device is not None:
            run([
                "rsync", "-lr",
                os.path.join(self.output, "../epics-base"),
                "root@{}:/opt".format(self.device),
            ])
            run([
                "rsync", "-lr",
                os.path.join(self.output, "release"),
                "root@{}:/opt/ioc".format(self.device),
            ])

    def test(self):
        self.manage(["test", "--tests", "unit"], "test/unit")
        self.manage(["test", "--tests", "integration"], "test/integration")
        if self.device is not None:
            self.deploy()
