from __future__ import annotations
from typing import Dict, List, Tuple

from random import Random
from pathlib import Path
from dataclasses import dataclass

from ferrite.codegen.base import CONTEXT, Context, Location, Name, Source, TestInfo, Type
from ferrite.codegen.structure import Field, Struct
from ferrite.codegen.variant import Variant


def make_variant(name: Name, messages: List[Tuple[Name, List[Field]]]) -> Variant:
    return Variant(
        name,
        [Field(suffux, Struct(Name(name, suffux), fields)) for suffux, fields in messages],
    )


@dataclass
class Generator:
    types: List[Type]

    def _set_context(self, context: Context) -> None:
        global CONTEXT
        for attr in dir(context):
            if attr.startswith('__'):
                continue
            setattr(CONTEXT, attr, getattr(context, attr))

    def generate(self, context: Context) -> Output:
        self._set_context(context)

        c_source = Source(Location.IMPORT, deps=[ty.c_source() for ty in self.types])
        rust_source = Source(Location.IMPORT, deps=[ty.rust_source() for ty in self.types])
        pyi_source = Source(Location.IMPORT, deps=[ty.pyi_source() for ty in self.types])

        return self.Output({
            Path(f"c/include/{context.prefix}.h"): "\n".join([
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
            Path(f"c/src/{context.prefix}.c"): "\n".join([
                f"#include <{context.prefix}.h>",
                "",
                c_source.make_source(Location.DEFINITION),
            ]),
            Path(f"rust/src/proto.rs"): "\n".join([
                "use flatty::make_flat;",
                rust_source.make_source(Location.IMPORT, separator=""),
                "",
                rust_source.make_source(Location.DECLARATION),
                "",
                rust_source.make_source(Location.DEFINITION),
            ]),
            Path(f"{context.prefix}.pyi"): "\n".join([
                "# This file was generatered by Ferrite Codegen."
                "from __future__ import annotations",
                "",
                pyi_source.make_source(Location.IMPORT, separator=""),
                "",
                pyi_source.make_source(Location.DECLARATION),
            ]),
        })

    def generate_tests(self, context: Context, info: TestInfo) -> Output:
        self._set_context(context)

        c_source = Source(Location.IMPORT, deps=[ty.c_test_source(info) for ty in self.types])
        rust_source = Source(Location.IMPORT, deps=[ty.rust_test_source(info) for ty in self.types])

        return self.Output({
            Path(f"c/src/test.c"): "\n".join([
                f"#include <{context.prefix}.h>",
                f"#include \"codegen_assert.h\"",
                "",
                c_source.make_source(Location.TEST),
            ]),
            Path(f"rust/src/tests.rs"): "\n".join([
                "#![allow(unused_variables, unused_imports)]",
                "",
                f"use std::os::raw::c_int;",
                f"use flatty::{{prelude::*, portable::NativeCast}};",
                f"use crate::*;",
                "",
                rust_source.make_source(Location.IMPORT, separator=""),
                "",
                rust_source.make_source(Location.TEST),
            ]),
        })

    @dataclass
    class Output:
        files: Dict[Path, str]

        def write(self, base_path: Path) -> None:
            base_path.mkdir(exist_ok=True)
            for rel_path, text in self.files.items():
                path = base_path / rel_path
                path.parent.mkdir(exist_ok=True, parents=True)
                content = text + "\n"
                old_content = None
                if path.exists():
                    with open(path, "r") as f:
                        old_content = f.read()
                if old_content is None or content != old_content:
                    with open(path, "w") as f:
                        f.write(content)

    def self_test(self, info: TestInfo) -> None:
        rng = Random(info.rng_seed)
        for ty in self.types:
            for i in range(info.attempts):
                data = ty.store(ty.random(rng))
                assert ty.store(ty.load(data)) == data
