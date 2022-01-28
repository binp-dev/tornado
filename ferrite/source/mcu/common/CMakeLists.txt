cmake_minimum_required(VERSION 3.16)

project("common")

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
set(SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/crc.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/crc.c"
)

add_library(${PROJECT_NAME} OBJECT ${SRC})
include_directories(${PROJECT_NAME} "include")
