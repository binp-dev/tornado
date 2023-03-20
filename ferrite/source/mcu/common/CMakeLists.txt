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

# Link IPP proto
add_subdirectory("../../ipp" "ipp")
include_directories("${IPP_GENERATED}/include")
target_link_libraries(${PROJECT_NAME} PUBLIC "ipp")

# Link HAL
add_subdirectory("../hal" "hal")
include_directories("../hal/include")
target_link_libraries(${PROJECT_NAME} PUBLIC ${HAL_TARGET})
