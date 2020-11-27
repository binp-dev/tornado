import os
import time
import logging as log

from script.util.subproc import run, SubprocError
from script.remote import Remote
from script import Component

import script.ioctool


class IocRunner:
    def __init__(self, device):
        self.device = device
        self.proc = None

    def __enter__(self):
        self.proc = Remote(self.device).popen([
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

    def _ioc_run_cmd(self, iocdtcmd, postfix, *args, **kwargs):
        iocdtcmd(
            top=self.path,
            output_dir=os.path.join(self.output, postfix),
            epics_base=self.tools["epics_base"].path,
            *args, **kwargs,
        )

    def _ioc_build(self, postfix, *args, **kwargs):
        self._ioc_run_cmd(script.ioctool.build, postfix, *args, **kwargs)

    def _ioc_test(self, postfix, *args, **kwargs):
        self._ioc_run_cmd(script.ioctool.test, postfix, *args, **kwargs)

    def _dev_run_cmd(self, args):
        Remote(self.device).run(args)

    def _dev_reboot(self):
        try:
            self._dev_run_cmd(["reboot", "now"])
        except:
            pass
        log.info("Waiting for SoC to reboot ...")
        time.sleep(10)
        for i in range(10-1, -1, -1):
            try:
                self._dev_run_cmd(["uname", "-a"])
            except SubprocError:
                if i > 0:
                    time.sleep(10)
                    continue
                else:
                    raise
            else:
                conn = True
                break

    def _dev_run_ioc(self):
        with IocRunner(self.device):
            try:
                while True:
                    time.sleep(1)
            except KeyboardInterrupt:
                pass

    def build(self):
        self._ioc_build("release")
        self._ioc_build("release", target="linux-arm")
    
    def clean(self):
        run(["rm", "-rf", self.output])

    def _deploy_epics(self):
        Remote(self.device).store(
            os.path.join(self.output, "../epics-base"),
            "/opt",
            r=True,
        )

    def deploy(self):
        self.build()
        if self.device is not None:
            dev = Remote(self.device)
            try:
                dev.run(["[[", "-d", "/opt/epics-base", "]]"])
            except SubprocError:
                self._deploy_epics()
            else:
                if self.args["update_epics"]:
                    self._deploy_epics()
            dev.store(
                os.path.join(self.output, "release"),
                "/opt/ioc",
                r=True,
            )
            dev.run([
                "sed", "-i",
                "'s/^epicsEnvSet(\"TOP\",.*)$/epicsEnvSet(\"TOP\",\"\\/opt\\/ioc\\/release\")/'",
                "/opt/ioc/release/iocBoot/iocPSC/envPaths"
            ])

    def test(self):
        if not self.args["no_local"]:
            self._ioc_test("test/unit", tests="unit")
            self._ioc_test("test/integration", tests="integration")
        if self.device is not None:
            self.deploy()
            self._dev_reboot()
            self._dev_run_ioc()
