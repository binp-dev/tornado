cmake_minimum_required(VERSION 3.16)

set(CMAKE_BUILD_TYPE "Debug" CACHE INTERNAL "")

set(CMAKE_C_FLAGS_DEBUG "-g -rdynamic -funwind-tables" CACHE INTERNAL "")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions" CACHE INTERNAL "")

set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*" CACHE INTERNAL "")
