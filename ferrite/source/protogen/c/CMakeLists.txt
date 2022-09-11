cmake_minimum_required(VERSION 3.16)

project("{{protogen}}")

add_library("{{protogen}}" OBJECT "include/{{protogen}}.h" "src/{{protogen}}.c")
target_include_directories("{{protogen}}" PUBLIC "include")

add_library("{{protogen}}_test" STATIC "src/codegen_assert.h" "src/test.c")
target_link_libraries("{{protogen}}_test" PUBLIC "{{protogen}}")
