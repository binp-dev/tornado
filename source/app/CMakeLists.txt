cmake_minimum_required(VERSION 3.16)

project("app_common")

add_subdirectory("../common" "common")

if(NOT DEFINED FERRITE)
    message(FATAL_ERROR "Variable 'FERRITE' is not defined")
endif()
if(NOT TARGET "app_base")
    add_subdirectory("${FERRITE}/app/base" "app_base")
endif()

if(NOT DEFINED IPP)
    message(FATAL_ERROR "Variable 'IPP' is not defined")
endif()
add_subdirectory(${IPP} "ipp")


set(SRC
    "src/device.hpp"
    "src/device.cpp"
    "src/handlers.hpp"
    "src/framework.cpp"
)

add_library(${PROJECT_NAME} OBJECT ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_link_libraries(${PROJECT_NAME} PUBLIC "common" "app_base" "ipp_cpp")
