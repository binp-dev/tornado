from __future__ import annotations
from typing import List, Tuple

from pathlib import Path

from ferrite.codegen.base import CONTEXT, Context, Location, Name, Source, Type
from ferrite.codegen.structure import Field, Struct
from ferrite.codegen.variant import Variant


def make_variant(name: Name, messages: List[Tuple[Name, List[Field]]]) -> Variant:
    return Variant(
        name,
        [Field(suffux, Struct(Name(name, suffux), fields)) for suffux, fields in messages],
    )


def generate_and_write(types: List[Type], base_path: Path, context: Context) -> None:
    for attr in dir(context):
        if attr.startswith('__'):
            continue
        setattr(CONTEXT, attr, getattr(context, attr))

    c_source = Source(Location.IMPORT, deps=[ty.c_source() for ty in types])
    rust_source = Source(Location.IMPORT, deps=[ty.rust_source() for ty in types])
    pyi_source = Source(Location.IMPORT, deps=[ty.pyi_source() for ty in types])

    files = {
        f"include/{context.prefix}.h": "\n".join([
            "#pragma once",
            ""
            "#include <stdlib.h>",
            "#include <stdint.h>",
            "#include <string.h>",
            "",
            c_source.make_source(Location.IMPORT, separator=""),
            "",
            "#ifdef __cplusplus",
            "extern \"C\" {",
            "#endif // __cplusplus",
            "",
            c_source.make_source(Location.DECLARATION),
            "",
            "#ifdef __cplusplus",
            "}",
            "#endif // __cplusplus",
        ]),
        f"src/{context.prefix}.c": "\n".join([
            f"#include <{context.prefix}.h>",
            "",
            c_source.make_source(Location.DEFINITION),
        ]),
        f"{context.prefix}.rs": "\n".join([
            rust_source.make_source(Location.IMPORT, separator=""),
            "",
            rust_source.make_source(Location.DECLARATION),
            "",
            rust_source.make_source(Location.DEFINITION),
        ]),
        f"{context.prefix}.pyi": "\n".join([
            "from __future__ import annotations",
            "",
            pyi_source.make_source(Location.IMPORT, separator=""),
            "",
            pyi_source.make_source(Location.DECLARATION),
        ]),
    }

    paths = [
        Path("include"),
        Path("src"),
    ]
    base_path.mkdir(exist_ok=True)
    for p in paths:
        (base_path / p).mkdir(exist_ok=True)
    for name, text in files.items():
        path = base_path / name
        content = text + "\n"
        old_content = None
        if path.exists():
            with open(path, "r") as f:
                old_content = f.read()
        if old_content is None or content != old_content:
            with open(path, "w") as f:
                f.write(content)
