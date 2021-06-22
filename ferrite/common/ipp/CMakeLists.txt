cmake_minimum_required(VERSION 3.16)

project("ipp")

set(CMAKE_C_FLAGS_DEBUG "-g -rdynamic -funwind-tables")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions -pedantic")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(
    "src"
)

set(SRC
    "src/ipp.c"
    "src/ipp.cpp"
)
set(UNITTEST_SRC
    ${SRC}
    "src/test.cpp"
)

enable_testing()
set(UNITTEST "${PROJECT_NAME}_unittest")
add_executable(${UNITTEST} ${UNITTEST_SRC})
target_compile_definitions(${UNITTEST} PRIVATE "UNITTEST")
target_link_libraries(${UNITTEST} PRIVATE "gtest" "pthread")
add_test(${UNITTEST} ${UNITTEST})
