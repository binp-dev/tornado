from __future__ import annotations

from enum import IntEnum
import logging


class LogLevel(IntEnum):
    TRACE = 0
    DEBUG = 1
    INFO = 2
    WARNING = 3
    ERROR = 4

    def level(self) -> int:
        return [
            logging.DEBUG,
            logging.DEBUG,
            logging.INFO,
            logging.WARNING,
            logging.ERROR,
        ][int(self)]

    def name(self) -> str:
        return [
            "trace",
            "debug",
            "info",
            "warn",
            "error",
        ][int(self)]
