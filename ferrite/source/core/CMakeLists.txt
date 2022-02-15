cmake_minimum_required(VERSION 3.16)

project("core")

set(SRC
    "src/core/assert.hpp"
    "src/core/cast.hpp"
    "src/core/io.hpp"
    "src/core/lazy_static.hpp"
    "src/core/maybe_uninit.hpp"
    "src/core/mutex.hpp"
    "src/core/panic.hpp"
    "src/core/panic.cpp"
    "src/core/result.hpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "src")
