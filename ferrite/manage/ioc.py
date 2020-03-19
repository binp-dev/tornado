import os
from subprocess import Popen
import time
import logging as log

from manage.util.subproc import run, SubprocError
from manage import Component


class IocRunner:
    def __init__(self, device):
        self.device = device
        self.proc = None

    def __enter__(self):
        self.proc = Popen([
            "ssh",
            "root@{}".format(self.device),
            "export {}; cd {} && {} {}".format(
                "LD_LIBRARY_PATH=/opt/epics-base/lib/linux-arm:/opt/ioc/release/lib/linux-arm",
                "/opt/ioc/release/iocBoot/iocPSC",
                "/opt/ioc/release/bin/linux-arm/PSC", "st.cmd"
            ),
        ])
        time.sleep(1)
        log.info("ioc '%s' started")

    def __exit__(self, *args):
        log.info("terminating '%s' ...")
        self.proc.terminate()
        log.info("ioc '%s' terminated")


class Ioc(Component):
    def setup(self, args, tools):
        self.path = os.path.join(args["top"], "ioc")
        self.output = os.path.join(args["output_dir"], "ioc")
        self.tools = tools
        self.device = args.get("dev_addr", None)
        self.args = args

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

    def run_cmd(self, args):
        run(["ssh", "root@{}".format(self.device)] + args)

    def reboot(self):
        try:
            self.run_cmd(["reboot", "now"])
        except:
            pass
        log.info("Waiting for SoC to reboot ...")
        time.sleep(10)
        for i in range(10-1, -1, -1):
            try:
                self.run_cmd(["uname", "-a"])
            except SubprocError:
                if i > 0:
                    time.sleep(10)
                    continue
                else:
                    raise
            else:
                conn = True
                break

    def run_ioc(self):
        with IocRunner(self.device):
            try:
                while True:
                    time.sleep(1)
            except KeyboardInterrupt:
                pass

    def build(self):
        self.manage(["build"], "release")
        self.manage(["build", "--target", "linux-arm"], "release")
    
    def clean(self):
        run(["rm", "-rf", self.output])

    def send_epics(self):
        run([
            "rsync", "-lr",
            os.path.join(self.output, "../epics-base"),
            "root@{}:/opt".format(self.device),
        ])

    def deploy(self):
        self.build()
        if self.device is not None:
            try:
                self.run_cmd(["[[", "-d", "/opt/epics-base", "]]"])
            except SubprocError:
                self.send_epics()
            else:
                if self.args["update_epics"]:
                    self.send_epics()
            run([
                "rsync", "-lr",
                os.path.join(self.output, "release"),
                "root@{}:/opt/ioc".format(self.device),
            ])
            self.run_cmd([
                "sed", "-i",
                "'s/^epicsEnvSet(\"TOP\",.*)$/epicsEnvSet(\"TOP\",\"\\/opt\\/ioc\\/release\")/'",
                "/opt/ioc/release/iocBoot/iocPSC/envPaths"
            ])

    def test(self):
        if not self.args["no_local"]:
            self.manage(["test", "--tests", "unit"], "test/unit")
            self.manage(["test", "--tests", "integration"], "test/integration")
        if self.device is not None:
            self.deploy()
            self.reboot()
            self.run_ioc()
