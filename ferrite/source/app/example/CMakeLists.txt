cmake_minimum_required(VERSION 3.16)

project("app_example")

add_subdirectory("../cmake/config" "config")
add_subdirectory("../base_rpmsg" "app_base_rpmsg")

include_directories("src")

set(SRC
    "src/framework.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE "core" "app_base" "app_base_rpmsg")
