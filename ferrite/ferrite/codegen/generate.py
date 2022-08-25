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

    c_source = Source(Location.IMPORT, deps=[ty.c_test_source() for ty in types])
    rust_source = Source(Location.IMPORT, deps=[ty.rust_test_source() for ty in types])
    pyi_source = Source(Location.IMPORT, deps=[ty.pyi_source() for ty in types])

    files = {
        f"c/include/{context.prefix}.h": "\n".join([
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
        f"c/src/{context.prefix}.c": "\n".join([
            f"#include <{context.prefix}.h>",
            "",
            c_source.make_source(Location.DEFINITION),
        ]),
        f"c/src/test.c": "\n".join([
            f"#include <{context.prefix}.h>",
            f"#include \"codegen_assert.h\"",
            "",
            c_source.make_source(Location.TEST),
        ]),
        f"rust/src/proto.rs": "\n".join([
            "use flatty::make_flat;",
            rust_source.make_source(Location.IMPORT, separator=""),
            "",
            rust_source.make_source(Location.DECLARATION),
            "",
            rust_source.make_source(Location.DEFINITION),
        ]),
        f"rust/src/tests.rs": "\n".join([
            "#![allow(unused_variables)]",
            "",
            f"//use std::os::raw::c_int;",
            f"use flatty::prelude::*;",
            f"use crate::*;",
            f"use crate::utils::vec_aligned;",
            "",
            rust_source.make_source(Location.IMPORT, separator=""),
            "",
            rust_source.make_source(Location.TEST),
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
        Path("c/include"),
        Path("c/src"),
        Path("rust/src"),
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
