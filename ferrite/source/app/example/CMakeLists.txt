cmake_minimum_required(VERSION 3.16)

project("app")

add_subdirectory("../app_rpmsg" "app_rpmsg")

include_directories("src")

set(SRC
    "src/framework.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE "app_rpmsg")
