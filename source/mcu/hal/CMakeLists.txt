cmake_minimum_required(VERSION 3.16)

project("hal")

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
set(COMMON_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/defs.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/panic.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/io.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/time.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rpmsg.c"
)
set(PLATFORM_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/io.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/rpmsg.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/spi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/gpt.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/gpio.c"
)

add_library(${PROJECT_NAME} OBJECT ${COMMON_SRC} ${PLATFORM_SRC})
