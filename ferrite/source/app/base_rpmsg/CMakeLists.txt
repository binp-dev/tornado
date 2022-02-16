cmake_minimum_required(VERSION 3.16)

project("app_base_rpmsg")

add_subdirectory("../base" "app_base")

set(SRC
    "src/channel/rpmsg.hpp"
    "src/channel/rpmsg.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_link_libraries(${PROJECT_NAME} PUBLIC "app_base")
