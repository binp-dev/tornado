import os
from typing import List, Tuple
from dataclasses import dataclass

from codegen.base import CONTEXT, Context, Location, Name, Source, Type
from codegen.struct import Field, Struct
from codegen.variant import Variant

def make_variant(name: Name, messages: List[Tuple[Name, List[Field]]]) -> Variant:
    return Variant(
        name,
        [
            Field(suffux, Struct(Name(name, suffux), fields))
            for suffux, fields in messages
        ],
    )

def generate_and_write(types: List[Type], base_path: str, context: Context):
    for attr in dir(context):
        if attr.startswith('__'):
            continue
        setattr(CONTEXT, attr, getattr(context, attr))

    c_source = Source(None, deps=[ty.c_source() for ty in types])
    cpp_source = Source(None, deps=[ty.cpp_source() for ty in types])
    test_source = Source(None, deps=[ty.test_source() for ty in types])

    files = {
        f"include/{context.prefix}.h": "\n".join([
            "#pragma once",
            ""
            "#include <stdlib.h>",
            "#include <stdint.h>",
            "#include <string.h>",
            "",
            c_source.make_source(Location.INCLUDES, separator="\n"),
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
        f"include/{context.prefix}.hpp": "\n".join([
            "#pragma once",
            "",
            cpp_source.make_source(Location.INCLUDES, separator="\n"),
            "",
            f"#include <{context.prefix}.h>",
            "",
            f"namespace {context.prefix} {{",
            "",
            cpp_source.make_source(Location.DECLARATION),
            "",
            f"}} // namespace {context.prefix}",
        ]),
        f"src/{context.prefix}.cpp": "\n".join([
            f"#include <{context.prefix}.hpp>",
            "",
            f"namespace {context.prefix} {{",
            "",
            cpp_source.make_source(Location.DEFINITION),
            "",
            f"}} // namespace {context.prefix}",
        ]),
        f"src/{context.prefix}_test.cpp": "\n".join([
            f"#include <{context.prefix}.hpp>",
            "",
            "#include <gtest/gtest.h>",
            "",
            f"using namespace {context.prefix};",
            "",
            test_source.make_source(Location.TESTS),
            "",
            "int main(int argc, char **argv) {",
            "    testing::InitGoogleTest(&argc, argv);",
            "    return RUN_ALL_TESTS();",
            "}",
        ]),
    }

    paths = [
        "include",
        "src",
    ]
    os.makedirs(base_path, exist_ok=True)
    for p in paths:
        os.makedirs(os.path.join(base_path, p), exist_ok=True)
    for name, text in files.items():
        path = os.path.join(base_path, name)
        content = text + "\n"
        old_content = None
        if os.path.exists(path):
            with open(path, "r") as f:
                old_content = f.read()
        if old_content is None or content != old_content:
            with open(path, "w") as f:
                f.write(content)