cmake_minimum_required(VERSION 3.16)

project("codegen")

set(SRC
    "include/codegen.h"
    "src/codegen.c"
)

add_library(${PROJECT_NAME} OBJECT ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "include")
