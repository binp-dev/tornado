cmake_minimum_required(VERSION 3.16)

project("ipp")

if(NOT DEFINED FERRITE)
    message(FATAL_ERROR "Variable 'FERRITE' is not defined")
endif()
if(NOT TARGET "core")
    add_subdirectory("${FERRITE}/core" "core")
endif()

set(SRC
    "include/ipp.h"
    "src/ipp.c"
)
set(SRC_CPP
    ${SRC}
    "include/ipp.hpp"
    "src/ipp.cpp"
)

add_library(${PROJECT_NAME} OBJECT ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "include")

set(PROJECT_CPP "${PROJECT_NAME}_cpp")
add_library(${PROJECT_CPP} OBJECT ${SRC_CPP})
target_include_directories(${PROJECT_CPP} PUBLIC "include")
target_link_libraries(${PROJECT_CPP} PRIVATE "core")
