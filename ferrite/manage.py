import os
import argparse
import logging as log

from component import epics_base, freertos, armgcc


def get_component(envvar, default, loader):
    try:
        path = os.environ[envvar]
        log.info("{} is set to '{}'".format(envvar, path))
    except KeyError:
        path = default
        log.info("{} is not set, using '{}'".format(envvar, path))

    loader.load(path)
    return path


class Manager:
    def __init__(self, outdir=None):
        self.components = {}

        if outdir is None:
            outdir = "output/epics-base"
        self.outdir = os.path.join(os.getcwd(), outdir)

    def load(self):
        self.components["armgcc_m4"] = get_component(
            "ARMGCC_DIR",
            os.path.join(os.getcwd(), "armgcc/m4"),
            armgcc.m4_loader,
        )
        self.components["armgcc_linux"] = get_component(
            "ARMGCC_LINUX_DIR",
            os.path.join(os.getcwd(), "armgcc/linux"),
            armgcc.linux_loader,
        )
        self.components["freertos"] = get_component(
            "FREERTOS_DIR",
            os.path.join(os.getcwd(), "freertos"),
            freertos.loader,
        )
        self.components["epics_base"] = get_component(
            "EPICS_BASE",
            os.path.join(os.getcwd(), "epics-base"),
            epics_base.EpicsLoader(self.components["armgcc_linux"], self.outdir),
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("-v", "--verbose", help="print additional information", action="store_true")
    parser.add_argument("--load", help="load third-party components", action="store_true")

    args = parser.parse_args()

    log.basicConfig(format='[%(levelname)s] %(message)s', level=log.DEBUG if args.verbose else log.INFO)


    mgr = Manager()

    if args.load:
        mgr.load()
