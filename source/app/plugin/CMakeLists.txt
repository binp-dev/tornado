cmake_minimum_required(VERSION 3.0)
project(app-plugin)

include_directories(
    "${EPICS_BASE}/include"
    "${EPICS_BASE}/include/os/Linux"
    "${EPICS_BASE}/include/compiler/gcc"
)

set(SRC
    "main.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC})
