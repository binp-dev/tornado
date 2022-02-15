cmake_minimum_required(VERSION 3.16)

project("app")

set(CMAKE_C_FLAGS_DEBUG "-g -rdynamic -funwind-tables")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

add_subdirectory("../app_rpmsg" "app_rpmsg")

include_directories("src")

set(SRC
    "src/framework.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE "app_rpmsg")
