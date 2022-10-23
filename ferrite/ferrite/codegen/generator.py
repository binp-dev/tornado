from __future__ import annotations
from typing import Any, Dict, List, Sequence, Tuple
from types import ModuleType

from random import Random
from pathlib import Path
from dataclasses import dataclass

from ferrite.codegen.base import Context, Location, Name, Source, TestInfo
from ferrite.codegen.types import Type, Field, Struct, Variant


def make_variant(name: Name, messages: List[Tuple[Name, List[Field]]]) -> Variant:
    return Variant(
        name,
        [Field(suffux, Struct(Name(name, suffux), fields)) for suffux, fields in messages],
    )


@dataclass
class Output:
    files: Dict[Path, str]

    def write(self, base_path: Path) -> None:
        base_path.mkdir(exist_ok=True, parents=True)
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


class Protogen:

    # `Type | type` is used here to suppress typing error while passing classes from generated `.pyi`.
    def __init__(self, types: Sequence[Type | type]) -> None:
        self.types: List[Type] = []
        for ty in types:
            assert isinstance(ty, Type)
            self.types.append(ty)

    def generate(self, context: Context) -> Output:
        context.set_global()

        c_source = Source(Location.IMPORT, deps=[ty.c_source() for ty in self.types])
        rust_source = Source(Location.IMPORT, deps=[ty.rust_source() for ty in self.types])
        pyi_source = Source(Location.IMPORT, deps=[ty.pyi_source() for ty in self.types])

        return Output({
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
                "use flatty::flat;",
                rust_source.make_source(Location.IMPORT, separator=""),
                "",
                rust_source.make_source(Location.DECLARATION),
                "",
                rust_source.make_source(Location.DEFINITION),
            ]),
            Path(f"{context.prefix}.pyi"): "\n".join([
                "# This file was generatered by Ferrite Protogen.",
                "from __future__ import annotations",
                "",
                "from ferrite.codegen.types import Value",
                "",
                pyi_source.make_source(Location.IMPORT, separator=""),
                "",
                pyi_source.make_source(Location.DECLARATION),
            ]),
        })

    def generate_tests(self, context: Context, info: TestInfo) -> Output:
        context.set_global()

        c_source = Source(Location.IMPORT, deps=[ty.c_test_source(info) for ty in self.types])
        rust_source = Source(Location.IMPORT, deps=[ty.rust_test_source(info) for ty in self.types])

        return Output({
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

    def self_test(self, info: TestInfo) -> None:
        Context("test", portable=True).set_global()

        rng = Random(info.rng_seed)
        for ty in self.types:
            for i in range(info.attempts):
                data = ty.store(ty.random(rng))
                assert ty.store(ty.load(data)) == data


Constant = Tuple[Type, Any]


class Configen:

    def __init__(self, module: ModuleType) -> None:
        self.module = module

    def _globals(self) -> Dict[str, Any]:
        return {k: getattr(self.module, k) for k in dir(self.module)}

    def typedefs(self) -> Dict[Name, Type]:
        out = {}
        for k, v in self._globals().items():
            if not k.startswith("_") and isinstance(v, Type):
                n = Name.from_camel(k)
                out[n] = v
        return out

    def constants(self) -> Dict[Name, Constant]:
        out = {}
        globals = self._globals()
        for k, ts in self.module.__annotations__.items():
            if not k.startswith("_"):
                t = eval(ts, globals)
                assert isinstance(t, Type)
                n = Name.from_snake(k.lower())
                v = globals[k]
                assert t.is_instance(v)
                out[n] = (t, v)
        return out

    def _deps(self) -> List[Type]:
        return [
            *self.typedefs().values(),
            *[t for t, _ in self.constants().values()],
        ]

    def c_source(self) -> Source:
        lines = []

        for n, t in self.typedefs().items():
            lines.append(f"typedef {t.c_type()} {n.camel()};")

        for n, (t, v) in self.constants().items():
            lines.append(f"#define {n.snake().upper()} (({t.c_type()}){t.c_object(v)})")

        return Source(
            Location.DECLARATION,
            [lines],
            deps=[d.c_source() for d in self._deps()],
        )

    def rust_source(self) -> Source:
        lines = []

        for n, t in self.typedefs().items():
            lines.append(f"pub type {n.camel()} = {t.rust_type()};")

        for n, (t, v) in self.constants().items():
            lines.append(f"pub const {n.snake().upper()}: {t.rust_type()} = {t.rust_object(v)};")

        return Source(
            Location.DECLARATION,
            [lines],
            deps=[d.rust_source() for d in self._deps()],
        )

    def generate(self, context: Context) -> Output:
        context.set_global()

        c_source = self.c_source()
        rust_source = self.rust_source()

        return Output({
            Path(f"c/config.h"): "\n".join([
                "#pragma once",
                ""
                "#include <stdlib.h>",
                "#include <stdint.h>",
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
            Path(f"rust/config.rs"): "\n".join([
                rust_source.make_source(Location.IMPORT, separator=""),
                "",
                rust_source.make_source(Location.DECLARATION),
            ]),
        })
