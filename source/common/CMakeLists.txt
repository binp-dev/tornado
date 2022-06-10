cmake_minimum_required(VERSION 3.16)

project("common")

set(SRC
    "include/common/config.h"
)

add_library(${PROJECT_NAME} OBJECT ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "include")
