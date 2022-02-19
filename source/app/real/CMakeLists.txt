cmake_minimum_required(VERSION 3.16)

project("app")

add_subdirectory("${FERRITE}/app/cmake/config" "config")
add_subdirectory("${FERRITE}/app/base_rpmsg" "app_base_rpmsg")
add_subdirectory(".." "app_common")

add_library(${PROJECT_NAME} SHARED "external.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE "core" "ipp_cpp" "app_base" "app_base_rpmsg" "app_common")
