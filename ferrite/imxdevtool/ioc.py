import os
from subprocess import Popen
import time
import logging as log

from imxdevtool.util.subproc import run, SubprocError
from imxdevtool import Component

import iocdevtool


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

    def ioc_run_cmd(self, iocdtcmd, postfix, *args, **kwargs):
        iocdtcmd(
            top=self.path,
            output_dir=os.path.join(self.output, postfix),
            epics_base=self.tools["epics_base"].path,
            *args, **kwargs,
        )

    def ioc_build(self, postfix, *args, **kwargs):
        self.ioc_run_cmd(iocdevtool.build, postfix, *args, **kwargs)

    def ioc_test(self, postfix, *args, **kwargs):
        self.ioc_run_cmd(iocdevtool.test, postfix, *args, **kwargs)

    def dev_run_cmd(self, args):
        run(["ssh", "root@{}".format(self.device)] + args)

    def dev_reboot(self):
        try:
            self.dev_run_cmd(["reboot", "now"])
        except:
            pass
        log.info("Waiting for SoC to reboot ...")
        time.sleep(10)
        for i in range(10-1, -1, -1):
            try:
                self.dev_run_cmd(["uname", "-a"])
            except SubprocError:
                if i > 0:
                    time.sleep(10)
                    continue
                else:
                    raise
            else:
                conn = True
                break

    def dev_run_ioc(self):
        with IocRunner(self.device):
            try:
                while True:
                    time.sleep(1)
            except KeyboardInterrupt:
                pass

    def build(self):
        self.ioc_build("release")
        self.ioc_build("release", target="linux-arm")
    
    def clean(self):
        run(["rm", "-rf", self.output])

    def deploy_epics(self):
        run([
            "rsync", "-lr",
            os.path.join(self.output, "../epics-base"),
            "root@{}:/opt".format(self.device),
        ])

    def deploy(self):
        self.build()
        if self.device is not None:
            try:
                self.dev_run_cmd(["[[", "-d", "/opt/epics-base", "]]"])
            except SubprocError:
                self.deploy_epics()
            else:
                if self.args["update_epics"]:
                    self.deploy_epics()
            run([
                "rsync", "-lr",
                os.path.join(self.output, "release"),
                "root@{}:/opt/ioc".format(self.device),
            ])
            self.dev_run_cmd([
                "sed", "-i",
                "'s/^epicsEnvSet(\"TOP\",.*)$/epicsEnvSet(\"TOP\",\"\\/opt\\/ioc\\/release\")/'",
                "/opt/ioc/release/iocBoot/iocPSC/envPaths"
            ])

    def test(self):
        if not self.args["no_local"]:
            self.ioc_test("test/unit", tests="unit")
            self.ioc_test("test/integration", tests="integration")
        if self.device is not None:
            self.deploy()
            self.dev_reboot()
            self.dev_run_ioc()
