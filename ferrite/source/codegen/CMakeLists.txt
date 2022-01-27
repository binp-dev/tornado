cmake_minimum_required(VERSION 3.16)

project("codegen")

set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions")

include_directories(
    "${CODEGEN_GENERATED}/include"
)

set(SRC
    "${CODEGEN_GENERATED}/include/codegen.h"
    "${CODEGEN_GENERATED}/src/codegen.c"
    "${CODEGEN_GENERATED}/include/codegen.hpp"
    "${CODEGEN_GENERATED}/src/codegen.cpp"
    "${CODEGEN_GENERATED}/src/codegen_test.cpp"
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS)

enable_testing()
set(TEST "${PROJECT_NAME}_test")
add_executable(${TEST} ${SRC})
set_property(TARGET ${TEST} PROPERTY CXX_STANDARD 17)
target_link_libraries(${TEST} PRIVATE "pthread" ${CONAN_LIBS})
add_test(${TEST} ${TEST})
