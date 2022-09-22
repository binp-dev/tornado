cmake_minimum_required(VERSION 3.16)

project("app")

add_subdirectory("${FERRITE}/app/cmake/config" "config")
add_subdirectory("${FERRITE}/app/base_zmq" "app_base_zmq")
add_subdirectory(".." "app_common")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS)

add_library(${PROJECT_NAME} SHARED "external.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE "core" "ipp_cpp" "app_base" "app_base_zmq" "app_common")
