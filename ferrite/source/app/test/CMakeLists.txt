cmake_minimum_required(VERSION 3.16)

project("app_base_test")

set(CMAKE_C_FLAGS_DEBUG "-g -rdynamic -funwind-tables")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

add_subdirectory("../zmq" "app_zmq")

set(SRC
    "src/zmq.cpp"
    "src/main.cpp"
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS)

enable_testing()
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE "core" "app_base" "app_zmq" "pthread" ${CONAN_LIBS})
add_test(${PROJECT_NAME} ${PROJECT_NAME})
