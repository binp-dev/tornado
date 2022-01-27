cmake_minimum_required(VERSION 3.16)

project("ipp")

if(NOT DEFINED IPP_GENERATED)
    message(FATAL_ERROR "IPP_GENERATED is not defined")
endif()

include_directories(
    "${IPP_GENERATED}/include"
)

set(SRC
    "${IPP_GENERATED}/include/ipp.h"
    "${IPP_GENERATED}/src/ipp.c"
)
set(SRC_CPP
    ${SRC}
    "${IPP_GENERATED}/include/ipp.hpp"
    "${IPP_GENERATED}/src/ipp.cpp"
)
set(SRC_TEST
    ${SRC_CPP}
    "${IPP_GENERATED}/src/ipp_test.cpp"
)

set(LIB ${PROJECT_NAME})
add_library(${LIB} OBJECT ${SRC})

set(LIB_CPP "${PROJECT_NAME}_cpp")
add_library(${LIB_CPP} OBJECT ${SRC_CPP})

if(${IPP_TEST})
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS)

    enable_testing()
    set(TEST "${PROJECT_NAME}_test")
    add_executable(${TEST} ${SRC_TEST})
    set_property(TARGET ${TEST} PROPERTY CXX_STANDARD 17)
    target_link_libraries(${TEST} PRIVATE "pthread" ${CONAN_LIBS})
    add_test(${TEST} ${TEST})
endif()
