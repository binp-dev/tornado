cmake_minimum_required(VERSION 3.0)

project(psc_ioc)

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions -pedantic")

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/PSCSup"
)

set(SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/PSCSup/channel/zmq_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PSCSup/unittest.cpp"
)

enable_testing()

add_executable("${PROJECT_NAME}"
    ${SOURCE}
)

target_compile_definitions("${PROJECT_NAME}" PRIVATE "DEBUG")

target_link_libraries("${PROJECT_NAME}" PRIVATE "gtest" "pthread")

add_test("${PROJECT_NAME}" "${PROJECT_NAME}")
