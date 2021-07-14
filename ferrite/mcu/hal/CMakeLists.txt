cmake_minimum_required(VERSION 3.16)

project("hal")

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
set(COMMON_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/defs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/rpmsg.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/gpt.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/spi.h"
)
set(IMX7_SRC
    ${COMMON_SRC}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/imx7/rpmsg.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/imx7/rpmsg.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/imx7/gpt.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/imx7/gpt.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/imx7/spi.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/imx7/spi.c"
)
set(IMX8MN_SRC
    ${COMMON_SRC}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/imx8mn/rpmsg.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/imx8mn/rpmsg.c"
)

set(IMX7_NAME "${PROJECT_NAME}_imx7")
add_library(${IMX7_NAME} OBJECT ${IMX7_SRC})
target_compile_definitions(${IMX7_NAME} PRIVATE "HAL_IMX7")

set(IMX8MN_NAME "${PROJECT_NAME}_imx8mn")
add_library(${IMX8MN_NAME} OBJECT ${IMX8MN_SRC})
target_compile_definitions(${IMX8MN_NAME} PRIVATE "HAL_IMX8MN")
