cmake_minimum_required(VERSION 3.0)

project(app)

set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions -pedantic")

include_directories(
    "src"
)

set(SOURCE
    "src/channel/zmq.cpp"
    "src/channel/rpmsg.cpp"
    "src/channel/zmq_test.cpp"
    "src/unittest.cpp"
)

enable_testing()

add_executable("${PROJECT_NAME}" ${SOURCE})

target_compile_definitions("${PROJECT_NAME}" PRIVATE "DEBUG")

target_link_libraries("${PROJECT_NAME}" PRIVATE "gtest" "pthread" "zmq")

add_test("${PROJECT_NAME}" "${PROJECT_NAME}")
