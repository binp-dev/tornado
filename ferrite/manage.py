import os

import epics.source


if __name__ == "__main__":
    dst = os.path.join(os.getcwd(), "epics-base")
    if os.path.exists(dst):
        assert os.path.isdir(dst)
    else:
        epics.source.load(dst)
    epics.source.armconf(dst, "/path/to/toolchain")
