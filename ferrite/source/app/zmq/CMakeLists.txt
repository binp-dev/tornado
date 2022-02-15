cmake_minimum_required(VERSION 3.16)

project("app_zmq")

add_subdirectory("../base" "app_base")

set(SRC
    "src/channel/zmq.hpp"
    "src/channel/zmq.cpp"
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS)

add_library(${PROJECT_NAME} STATIC ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_link_libraries(${PROJECT_NAME} PUBLIC "app_base" ${CONAN_LIBS})
