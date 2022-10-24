from __future__ import annotations

from ferrite.utils.path import TargetPath
from ferrite.components import codegen
from ferrite.codegen import generator

from tornado import config


class Config(codegen.Configen):

    def __init__(self) -> None:
        super().__init__(
            "config",
            TargetPath("tornado/config"),
            generator.Configen(config),
        )
