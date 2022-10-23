from __future__ import annotations

from pathlib import Path

from ferrite.utils.path import TargetPath
from ferrite.components.codegen import ProtogenTest
from ferrite.components.rust import RustcHost
from ferrite.codegen.generator import Generator

from example.protocol import InMsg, OutMsg


class Protocol(ProtogenTest):

    def __init__(self, rustc: RustcHost):
        super().__init__("protocol", TargetPath("example/protocol"), Generator([InMsg, OutMsg]), rustc)
