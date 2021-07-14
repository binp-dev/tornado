cmake_minimum_required(VERSION 3.16)

project("common")

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
set(SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/common/main.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
)

add_library(${PROJECT_NAME} MODULE ${SRC})
include_directories(${PROJECT_NAME} "include")

# Link IPP proto
add_subdirectory("../../common/ipp" "ipp")
include_directories("../../common/ipp/src")
target_link_libraries(${PROJECT_NAME} PRIVATE "ipp")

# Link HAL
add_subdirectory("../hal" "hal")
include_directories("../hal/src")
target_link_libraries(${PROJECT_NAME} PRIVATE "hal_imx7")
