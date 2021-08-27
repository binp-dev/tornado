from __future__ import annotations
from dataclasses import dataclass

from ipp.base import Type, Source

class LoadStatus(Type):
    def __init__(self):
        super().__init__()

    def name(self) -> str:
        return "LoadStatus"

    def c_type(self) -> str:
        return "LoadStatus"

    def c_source(self) -> Source:
        return Source(["\n".join([
            f"//! @brief IPP load message status",
            f"typedef enum {{",
            f"    IPP_LOAD_OK                 = 0x00, /* ok */",
            f"    IPP_LOAD_OUT_OF_BOUNDS      = 0x01, /* max buffer length is too short */",
            f"    IPP_LOAD_PARSE_ERROR        = 0x02, /* message parse error */",
            f"}} {self.c_type()};",
        ])])

class StoreStatus(Type):
    def __init__(self):
        super().__init__()

    def name(self) -> str:
        return "StoreStatus"

    def c_type(self) -> str:
        return "StoreStatus"

    def c_source(self) -> Source:
        return Source(["\n".join([
            f"//! @brief IPP load message status",
            f"typedef enum {{",
            f"    IPP_STORE_OK                 = 0x00, /* ok */",
            f"    IPP_STORE_OUT_OF_BOUNDS      = 0x01, /* max buffer length is too short */",
            f"}} {self.c_type()};",
        ])])

@dataclass
class LoadFn:
    type: Type

    def c_name(self):
        return f"{self.type.name()}_load"

    def c_decl(self):
        return "".join([
            f"{LoadStatus().c_type()} {self.c_name()}(",
            f"{self.type.c_type()} * dst, ",
            f"const uint8_t * src, ",
            f"size_t max_len",
            f")",
        ])

    def c_call(self, dst: str, src: str, max_len: str):
        return f"{self.c_name()}({src}, {dst}, {max_len})"

@dataclass
class StoreFn:
    type: Type

    def c_name(self):
        return f"{self.type.name()}_store"

    def c_decl(self):
        return "".join([
            f"{StoreStatus().c_type()} {self.c_name()}(",
            f"const {self.type.c_type()} * src, ",
            f"uint8_t * dst, ",
            f"size_t max_len, ",
            f"size_t * len",
            f")",
        ])

    def c_call(self, dst: str, src: str, max_len: str, len: str):
        return f"{self.c_name()}({src}, {dst}, {max_len}, {len})"
