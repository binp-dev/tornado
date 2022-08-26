cmake_minimum_required(VERSION 3.16)

project("codegen")

add_library("codegen" OBJECT "include/codegen.h" "src/codegen.c")
target_include_directories("codegen" PUBLIC "include")

add_library("codegen_test" STATIC "src/codegen_assert.h" "src/test.c")
target_link_libraries("codegen_test" PUBLIC "codegen")
