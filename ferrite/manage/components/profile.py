from __future__ import annotations
import logging
from manage.components.toolchains import Toolchain

class Profile(object):
    def __init__(self, app: Toolchain, mcu: Toolchain):
        # If `app_toolchain` is None then use host environment.
        # If `mcu_toolchain` is None then don't use it.
        super().__init__()
        self.device = device
