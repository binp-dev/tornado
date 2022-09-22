from __future__ import annotations

from pathlib import Path

from ferrite.utils.path import TargetPath
from ferrite.components.protogen import ProtogenTest
from ferrite.components.rust import RustcHost
from ferrite.protogen.generator import Generator

from tornado.ipp import AppMsg, McuMsg


class Ipp(ProtogenTest):

    def __init__(self, rustc: RustcHost):
        super().__init__("ipp", TargetPath("tornado/ipp"), Generator([AppMsg, McuMsg]), True, rustc)
