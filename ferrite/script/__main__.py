import sys, os
import argparse
import logging as log

from script.toolchain import epics_base, freertos, armgcc
from script import ioc, mcu

log.basicConfig(format='[%(levelname)s] %(message)s', level=log.DEBUG)


class Tool:
    def __init__(self):
        self.path = None
        self.loader = None

    def set_path(self, path, env, default):
        if path is None:
            try:
                path = os.environ[env]
                log.info("{} is set to '{}'".format(env, path))
            except KeyError:
                path = default
                log.info("{} is not set, using '{}'".format(env, path))
        self.path = path

    def load(self):
        self.loader.load(self.path)


class ArmgccMcu(Tool):
    def __init__(self):
        super().__init__()

    def locate(self, args):
        self.set_path(
            args.get("armgcc_mcu", None),
            "ARMGCC_MCU",
            os.path.join(args["top"], "toolchain", "armgcc/mcu"),
        )
        self.loader = armgcc.mcu_loader

class ArmgccApp(Tool):
    def __init__(self):
        super().__init__()

    def locate(self, args):
        self.set_path(
            args.get("armgcc_app", None),
            "ARMGCC_APP",
            os.path.join(args["top"], "toolchain", "armgcc/app"),
        )
        self.loader = armgcc.app_loader

class Freertos(Tool):
    def __init__(self):
        super().__init__()

    def locate(self, args):
        self.set_path(
            args.get("freertos", None),
            "FREERTOS",
            os.path.join(args["top"], "toolchain", "freertos"),
        )
        self.loader = freertos.loader

class EpicsBase(Tool):
    def __init__(self, armgcc):
        super().__init__()
        self.armgcc = armgcc

    def locate(self, args):
        self.set_path(
            args.get("epics_base", None),
            "EPICS_BASE",
            os.path.join(args["top"], "toolchain", "epics-base"),
        )
        self.outdir = os.path.join(args["output_dir"], "epics-base")
        self.loader = epics_base.EpicsLoader(self.armgcc.path, self.outdir)


tools = {
    "armgcc_mcu":    ArmgccMcu(),
    "armgcc_app":    ArmgccApp(),
    "freertos":      Freertos(),
}
tools["epics_base"] = EpicsBase(tools["armgcc_app"])

components = [
    ("mcu", mcu.Mcu()),
    ("ioc", ioc.Ioc()),
]

class Handler:
    def __init__(self):
        self.parser = self._create_parser()
        self.components = []

    def _create_parser(self):
        parser = argparse.ArgumentParser()
        parser.add_argument(
            "--top", metavar="PATH", type=str,
            help="Absolute path to the project top directory. By default it is current directory.",
        )
        parser.add_argument(
            "--output-dir", metavar="PATH", type=str,
            help=" ".join([
                "Absolute path to directory where binaries will be stored after build.",
                "By default 'build' directory is used",
            ]),
        )
        parser.add_argument(
            "--components", metavar="A,B,C", type=str, default="all",
            help="\n".join([
                "List of comma-separated components to run command on. All by default.",
                "Available components are: {}.".format(", ".join([c[0] for c in components])),
            ]),
        )
        return parser

    def _handle_args(self, args):
        if args["top"] is None:
            args["top"] = os.getcwd()
        if args["output_dir"] is None:
            args["output_dir"] = os.path.join(args["top"], "build")
        
        if args["components"] == "all":
            self.components = components
        else:
            comp_list = args["components"].split(",")
            assert len(set(comp_list) - set([c[0] for c in components])) == 0
            self.components = [c for c in components if c[0] in comp_list]

        for c in tools.values():
            c.locate(args)

        for _, c in self.components:
            c.setup(args, tools)

        return args

    def _run(self, args):
        raise NotImplementedError()

    def __call__(self, argv):
        args = vars(self.parser.parse_args(argv))
        args = self._handle_args(args)
        self._run(args)


class LoadHandler(Handler):
    def __init__(self):
        super().__init__()

    def _create_parser(self):
        parser = argparse.ArgumentParser(
            description="Downloads and prepare third-party tools",
            parents=[super()._create_parser()], add_help=False,
        )
        parser.add_argument(
            "--tools", action="append", type=str,
            choices=list(tools.keys()), default=[],
            help="Select tools to download. All by default."
        )
        return parser
    
    def _handle_args(self, args):
        args = super()._handle_args(args)

        if not args["tools"]:
            args["tools"] = list(tools.keys())
        log.info("Tools selected: {}".format(args["tools"]))

        return args

    def _run(self, args):
        for c in args["tools"]:
            tools[c].load()


class BuildHandler(Handler):
    def __init__(self):
        super().__init__()

    def _create_parser(self):
        parser = argparse.ArgumentParser(
            description="Build the software",
            parents=[super()._create_parser()], add_help=False,
        )
        return parser
    
    def _handle_args(self, args):
        args = super()._handle_args(args)
        return args

    def _run(self, args):
        for _, c in self.components:
            c.build()


class CleanHandler(Handler):
    def __init__(self):
        super().__init__()

    def _create_parser(self):
        parser = argparse.ArgumentParser(
            description="Clean intermediate files created during build",
            parents=[super()._create_parser()], add_help=False,
        )
        return parser
    
    def _handle_args(self, args):
        args = super()._handle_args(args)
        return args

    def _run(self, args):
        for _, c in self.components:
            c.clean()


class DeployHandler(BuildHandler):
    def __init__(self):
        super().__init__()

    def _create_parser(self):
        parser = argparse.ArgumentParser(
            description="Deploy software to the device",
            parents=[super()._create_parser()], add_help=False,
        )
        parser.add_argument(
            "--dev-addr", type=str, metavar="ADDR[:PORT]",
            help=" ".join([
                "Specify device IP address to connect via SSH.",
                "The SSH should be configured to allow login as `root` without prompt for password."
            ]),
        )
        parser.add_argument(
            "--no-dev", action="store_true",
            help="Perform only local tasks (if there is no device accessible).",
        )
        parser.add_argument(
            "--update-epics", action="store_true",
            help="Update epics-base directory on the device.",
        )
        return parser
    
    def _handle_args(self, args):
        args = super()._handle_args(args)

        if not args["dev_addr"] and not args["no_dev"]:
            raise AssertionError("You should specify either `--dev-addr` or `--no-dev`.")

        return args

    def _run(self, args):
        for _, c in self.components:
            c.deploy()


class TestHandler(DeployHandler):
    def __init__(self):
        super().__init__()

    def _create_parser(self):
        parser = argparse.ArgumentParser(
            description="Test software both locally and on the device",
            parents=[super()._create_parser()], add_help=False,
        )
        parser.add_argument(
            "--no-local", action="store_true",
            help="Does not perform local testing.",
        )
        return parser
    
    def _handle_args(self, args):
        args = super()._handle_args(args)
        return args

    def _run(self, args):
        for _, c in self.components:
            c.test()


commands = {
    "load":     LoadHandler(),
    "build":    BuildHandler(),
    "clean":    CleanHandler(),
    "deploy":   DeployHandler(),
    "test":     TestHandler(),
}

command_parser = argparse.ArgumentParser(
    description="PSC software building and testing tool",
    usage="python3 -m script <command> [options...]",
)
command_parser.add_argument(
    "command", choices=commands.keys(),
    help=" ".join([
        "Task that will be performed by this script.",
        "To read about specific command use `<command> --help`"
    ]),
)


args = command_parser.parse_args(sys.argv[1:2])

commands[args.command](sys.argv[2:])
