import os
from argparse import ArgumentParser


from manage.build import build, clean
from manage.test import test

tasks = {
    "build": build,
    "clean": clean,
    "test":  test,
}

if __name__ == "__main__":
    parser = ArgumentParser(
        description="IOC management tool",
        usage="python3 manage.py <command> [options...]",
    )

    parser.add_argument(
        "command", choices=tasks.keys(),
        help="Task that will be performed by this script.",
    )
    parser.add_argument(
        "--epics-base", metavar="PATH",
        help=" ".join([
            "Absolute path to EPICS base directory.",
            "If not specified then EPICS_BASE environment variable is used.",
        ]),
    )
    parser.add_argument(
        "--top", metavar="PATH",
        help=" ".join([
            "Absolute path to IOC top directory.",
            "If not specified then TOP environment variable is used.",
            "If TOP also isn't specified then current directory is used.",
        ]),
    )
    parser.add_argument(
        "--threads", metavar="N", type=int,
        help="Number of threads for compilation, '-jN' option of 'make'. Default value is 1.",
    )
    parser.add_argument(
        "--target", metavar="ARCH",
        help=" ".join([
            "Target architecture for cross-compilation, e.g. 'linux-arm'.",
            "By default IOC will be compiled for host architecture.",
        ]),
    )
    parser.add_argument(
        "--output-dir", metavar="PATH",
        help=" ".join([
            "Absolute path to directory where IOC binaries will be stored after compilation.",
            "By default TOP is used for release builds, 'build/*test' is used for test builds",
        ]),
    )
    parser.add_argument(
        "--tests", choices=["all", "unit", "integration"],
        help="Which kind of testing should be run. All by default.",
    )

    args = vars(parser.parse_args())


    command = args["command"]
    del args["command"]

    if args["epics_base"] is None:
        try:
            args["epics_base"] = os.environ["EPICS_BASE"]
        except KeyError:
            print(" ".join([
                "error:",
                "Either '--epics-base' argument or 'EPICS_BASE' environment variable should be specified",
            ]))
            exit(1)

    if args["top"] is None:
        try:
            args["top"] = os.environ["TOP"]
        except KeyError:
            args["top"] = os.getcwd()


    if command == "test":
        if args["target"] is not None:
            print(" ".join([
                "warning:",
                "Unnecessary argument '--target' for task 'test'.",
                "Test could be run only on host architecture.",
            ]))
        del args["target"]

        if args["tests"] is None:
            args["tests"] = "all"
    else:
        if args["tests"] is not None:
            print("warning: Unnecessary argument '--tests' for task '{}'.".format(command))
        del args["tests"]


    tasks[command](**args)
