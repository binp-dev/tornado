cmake_minimum_required(VERSION 3.16)

project("codegen")

add_subdirectory(${CORE_DIR} "core")

set(SRC
    "include/codegen.h"
    "src/codegen.c"
    "include/codegen.hpp"
    "src/codegen.cpp"
)

add_library(${PROJECT_NAME} OBJECT ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "include")
target_link_libraries(${PROJECT_NAME} PUBLIC "core")
