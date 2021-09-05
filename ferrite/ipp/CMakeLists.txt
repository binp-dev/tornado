cmake_minimum_required(VERSION 3.16)

project("ipp")

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

enable_testing()
set(UNITTEST "${PROJECT_NAME}_test")
add_executable(${UNITTEST} ${SRC_TEST})
set_property(TARGET ${UNITTEST} PROPERTY CXX_STANDARD 17)
target_link_libraries(${UNITTEST} PRIVATE "gtest" "pthread")
add_test(${UNITTEST} ${UNITTEST})
