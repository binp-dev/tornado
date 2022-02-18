cmake_minimum_required(VERSION 3.16)

project("core")

set(CMAKE_C_FLAGS_DEBUG "-g -rdynamic -funwind-tables" CACHE INTERNAL "")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions" CACHE INTERNAL "")
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*" CACHE INTERNAL "")

set(SRC
    "src/core/assert.hpp"
    "src/core/convert.hpp"
    "src/core/io.hpp"
    "src/core/io.cpp"
    "src/core/lazy_static.hpp"
    "src/core/maybe_uninit.hpp"
    "src/core/mutex.hpp"
    "src/core/panic.hpp"
    "src/core/panic.cpp"
    "src/core/result.hpp"
)

add_library(${PROJECT_NAME} OBJECT ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "src")
