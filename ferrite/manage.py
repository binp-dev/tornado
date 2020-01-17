import sys, os
from subprocess import run
from argparse import ArgumentParser
from urllib.request import urlretrieve


class Manager:
    def __init__(self, cwd):
        self.cwd = cwd

    def build(self, epics_base, target=None, output_dir=None, clean=False, threads=None):
        args = ["make"]

        if clean:
            args += ["clean", "uninstall"]

        if threads is not None:
            args += ["-j{}".format(threads)]

        args += ["EPICS_BASE={}".format(epics_base)]
        if target is not None:
            args += ["CROSS_COMPILER_TARGET_ARCHS={}".format(target)]
        if output_dir is not None:
            args += ["INSTALL_LOCATION={}".format(output_dir)]
        
        run(args, cwd=self.cwd, check=True)

        if output_dir is not None:
            if not clean:
                run(["cp", "-r", "iocBoot", output_dir], cwd=self.cwd, check=True)
            else:
                run(["rm", "-r", output_dir], cwd=self.cwd)


    def test(self, **kwargs):
        if "target" in kwargs:
            raise TypeError("test() got an unexpected keyword argument 'target'")

        run(
            ["make", "test", "EPICS_BASE={}".format(kwargs["epics_base"])],
            cwd=os.path.join(self.cwd, "unittest"),
            check=True,
        )

if __name__ == "__main__":
    parser = ArgumentParser(
        description="IOC management tool",
        usage="python3 manage.py <command> [options...]",
    )
    parser.add_argument(
        "command", choices=["build", "clean", "test"],
        help="Task that will be performed by this script.",
    )
    parser.add_argument(
        "--epics-base", metavar="PATH",
        help="Absolute path to EPICS base directory. If not specified then EPICS_BASE environment variable is used.",
    )
    #parser.add_argument(
    #    "--threads", metavar="N", type=int,
    #    help="Number of threads for compilation, '-jN' option of 'make'. Default value is 1.",
    #)
    parser.add_argument(
        "--target", metavar="ARCH",
        help="Target architecture for cross-compilation, e.g. 'linux-arm'. By default IOC will be compiled for host architecture.",
    )
    parser.add_argument(
        "--output-dir", metavar="PATH",
        help="Absolute path to directory where IOC binaries will be stored after compilation. By default IOC source directory is used.",
    )

    args = vars(parser.parse_args())
    
    command = args["command"]
    del args["command"]

    if args["epics_base"] is None:
        try:
            args["epics_base"] = os.environ["EPICS_BASE"]
        except KeyError:
            print("error: Either '--epics-base' argument or 'EPICS_BASE' environment variable should be specified")
            exit(1)

    if command == "test":
        if args["target"] is not None:
            print("Unnecessary argument '--target' for task 'test'. Test could be run only on host architecture.")
        del args["target"]

    if command == "clean":
        command = "build"
        args["clean"] = True

    getattr(Manager(os.getcwd()), command)(**args)
