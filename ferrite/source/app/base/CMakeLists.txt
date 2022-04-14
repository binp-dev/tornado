cmake_minimum_required(VERSION 3.16)

project("app_base")

if(NOT TARGET "core")
    add_subdirectory("../../core" "core")
endif()

set(SRC
    "src/record/array.hpp"
    "src/record/base.hpp"
    "src/record/value.hpp"

    "src/channel/base.hpp"
    "src/channel/message.hpp"
    "src/channel/message.hxx"
)

add_library(${PROJECT_NAME} OBJECT ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_link_libraries(${PROJECT_NAME} PUBLIC "core")
