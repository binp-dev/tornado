import sys, os
import argparse
import logging as log

from manage.components import epics_base, freertos, armgcc


class Component:
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


class ArmgccM4(Component):
    def __init__(self):
        super().__init__()

    def locate(self, args):
        self.set_path(
            args.get("armgcc_m4", None),
            "ARMGCC_M4",
            os.path.join(args["top"], "components", "armgcc/m4"),
        )
        self.loader = armgcc.m4_loader

class ArmgccLinux(Component):
    def __init__(self):
        super().__init__()

    def locate(self, args):
        self.set_path(
            args.get("armgcc_linux", None),
            "ARMGCC_LINUX",
            os.path.join(args["top"], "components", "armgcc/linux"),
        )
        self.loader = armgcc.linux_loader

class Freertos(Component):
    def __init__(self):
        super().__init__()

    def locate(self, args):
        self.set_path(
            args.get("freertos", None),
            "FREERTOS",
            os.path.join(args["top"], "components", "freertos"),
        )
        self.loader = freertos.loader

class EpicsBase(Component):
    def __init__(self, armgcc):
        super().__init__()
        self.armgcc = armgcc

    def locate(self, args):
        self.set_path(
            args.get("epics_base", None),
            "EPICS_BASE",
            os.path.join(args["top"], "components", "epics-base"),
        )
        self.outdir = args["output_dir"]
        self.loader = epics_base.EpicsLoader(self.armgcc.path, self.outdir)


components = {
    "armgcc_m4":    ArmgccM4(),
    "armgcc_linux": ArmgccLinux(),
    "freertos":     Freertos(),
}
components["epics_base"] = EpicsBase(components["armgcc_linux"])


class Handler:
    def __init__(self):
        self.parser = self._create_parser()

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
                "By default 'output' directory is used",
            ]),
        )
        return parser

    def _handle_args(self, args):
        if args["top"] is None:
            args["top"] = os.getcwd()
        if args["output_dir"] is None:
            args["output_dir"] = os.path.join(args["top"], "output")

        for k, c in components.items():
            args[k] = c.locate(args)

        return args

    def _run(self, args):
        print(args)

    def __call__(self, argv):
        args = vars(self.parser.parse_args(argv))
        args = self._handle_args(args)
        self._run(args)


class LoadHandler(Handler):
    def __init__(self):
        super().__init__()

    def _create_parser(self):
        parser = argparse.ArgumentParser(
            description="Downloads and prepare third-party components",
            parents=[super()._create_parser()], add_help=False,
        )
        parser.add_argument(
            "--component", action="append", type=str,
            choices=list(components.keys()), default=list(components.keys()),
            help="Select components to download. All by default."
        )
        return parser
    
    def _handle_args(self, args):
        args = super()._handle_args(args)

        if not args["component"]:
            args["component"] = list(components.keys())

        return args

    def _run(self, args):
        super()._run(args)

        for c in args["component"]:
            components[c].load()


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
        return args

    def _run(self, args):
        pass


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
        return args

    def _run(self, args):
        pass


class DeployHandler(BuildHandler):
    def __init__(self):
        super().__init__()

    def _create_parser(self):
        parser = argparse.ArgumentParser(
            description="Deploy software to the device",
            parents=[super()._create_parser()], add_help=False,
        )
        return parser
    
    def _handle_args(self, args):
        return args

    def _run(self, args):
        pass


class TestHandler(DeployHandler):
    def __init__(self):
        super().__init__()

    def _create_parser(self):
        parser = argparse.ArgumentParser(
            description="Test software both locally and on the device",
            parents=[super()._create_parser()], add_help=False,
        )
        return parser
    
    def _handle_args(self, args):
        return args

    def _run(self, args):
        pass

commands = {
    "load":     LoadHandler(),
    "build":    BuildHandler(),
    "clean":    CleanHandler(),
    "deploy":   DeployHandler(),
    "test":     TestHandler(),
}

command_parser = argparse.ArgumentParser(
    description="PSC software building and testing tool",
    usage="python3 -m manage <command> [options...]",
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
