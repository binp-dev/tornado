from __future__ import annotations
from typing import Callable, List

from enum import Enum

from ferrite.utils.strings import quote

from ferrite.codegen.utils import hash_str

OK = "core::Ok"
ERR = "core::Err"
MONOSTATE = "std::monostate"


def monostate() -> str:
    return f"{MONOSTATE}()"


def ok(value: str = monostate()) -> str:
    return f"{OK}({value})"


def err(value: str) -> str:
    return f"{ERR}({value})"


def io_read_type() -> str:
    return "core::io::StreamReadExact"


def io_write_type() -> str:
    return "core::io::StreamWriteExact"


def io_result_type(tys: str = MONOSTATE) -> str:
    return f"core::Result<{tys}, core::io::Error>"


def try_unwrap(expr: str, op: Callable[[str], str] | None = None) -> List[str]:
    res = f"res_{hash_str((op('') if op is not None else '') + expr)[:4]}"
    return [
        f"auto {res} = {expr};",
        f"if ({res}.is_err()) {{ return {err(f'{res}.unwrap_err()')}; }}",
        *([op(f"{res}.unwrap()")] if op is not None else []),
    ]


def stream_read(stream: str, dst_ptr: str, size: int | str, cast: bool = True) -> str:
    if cast:
        dst_ptr = f"reinterpret_cast<uint8_t *>({dst_ptr})"
    return f"{stream}.stream_read_exact({dst_ptr}, {size})"


def stream_write(stream: str, src_ptr: str, size: int | str, cast: bool = True) -> str:
    if cast:
        src_ptr = f"reinterpret_cast<const uint8_t *>({src_ptr})"
    return f"{stream}.stream_write_exact({src_ptr}, {size})"


class ErrorKind(Enum):
    UNEXPECTED_EOF = 1,
    INVALID_DATA = 2,

    def cpp_name(self) -> str:
        pref = "core::io::ErrorKind::"
        if self == ErrorKind.UNEXPECTED_EOF:
            post = "UnexpectedEof"
        elif self == ErrorKind.INVALID_DATA:
            post = "InvalidData"
        return f"{pref}{post}"


def io_error(kind: ErrorKind, desc: str | None = None) -> str:
    if desc is not None:
        msg_arg = f", \"{quote(desc)}\""
    else:
        msg_arg = ""
    return f"core::io::Error{{{kind.cpp_name()}{msg_arg}}}"
