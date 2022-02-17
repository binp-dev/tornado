from __future__ import annotations
from typing import Any, Callable, List

from enum import Enum

from ferrite.utils.strings import quote

from ferrite.codegen.base import Type
from ferrite.codegen.utils import hash_str, indent, to_ident


def io_read_type() -> str:
    return "io::Read"


def io_write_type() -> str:
    return "io::Write"


def io_result_type(tys: Type[Any] | str = "std::monostate") -> str:
    if isinstance(tys, Type):
        tys = tys.cpp_type()
    return f"Result<{tys}, io::Error>"


def try_unwrap(expr: str, op: Callable[[str], str] | None = None) -> List[str]:
    res = f"res_{hash_str((op('') if op is not None else '') + expr)[:4]}"
    return [
        f"auto {res} = {expr};",
        f"if ({res}.is_err()) {{ return Err({res}.unwrap_err()); }}",
        *([op(f"{res}.unwrap()")] if op is not None else []),
    ]


class ErrorKind(Enum):
    END_OF_STREAM = 1,

    def cpp_name(self) -> str:
        pref = "io::ErrorKind::"
        if self == ErrorKind.END_OF_STREAM:
            post = "END_OF_STREAM"
        return f"{pref}{post}"


def io_error(kind: ErrorKind, desc: str | None = None) -> str:
    if desc is not None:
        msg_arg = f", \"{quote(desc)}\""
    else:
        msg_arg = ""
    return f"io::Error({kind.cpp_name()}{msg_arg})"
