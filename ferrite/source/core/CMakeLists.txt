cmake_minimum_required(VERSION 3.16)

project("core")

set(SRC
    "src/core/assert.hpp"
    "src/core/convert.hpp"
    "src/core/numeric.hpp"
    "src/core/io.hpp"
    "src/core/io.cpp"
    "src/core/lazy_static.hpp"
    "src/core/maybe_uninit.hpp"
    "src/core/match.hpp"
    "src/core/mutex.hpp"
    "src/core/panic.hpp"
    "src/core/panic.cpp"
    "src/core/result.hpp"
    "src/core/collections/slice.hpp"
    "src/core/collections/vec.hpp"
    "src/core/collections/vec_deque.hpp"
    "src/core/collections/vec_deque.hxx"
    "src/core/collections/vec_deque.cpp"
)

add_library(${PROJECT_NAME} OBJECT ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "src")
