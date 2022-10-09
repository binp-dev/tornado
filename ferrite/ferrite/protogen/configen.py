from __future__ import annotations
from typing import List, Dict, Any, Tuple

from pathlib import Path

from ferrite.protogen.base import Name, Context, Type, Source, Location
from ferrite.protogen.generator import Output

Constant = Tuple[Type, Any]


class Config:

    def typedefs(self) -> Dict[Name, Type]:
        raise NotImplementedError()

    def constants(self) -> Dict[Name, Constant]:
        raise NotImplementedError()

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
            deps=[d.c_source() for d in self._deps()],
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
