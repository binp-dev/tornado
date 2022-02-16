cmake_minimum_required(VERSION 3.16)

project("app_base_test")

add_subdirectory("../base_zmq" "app_base_zmq")

set(SRC
    "src/zmq.cpp"
    "src/main.cpp"
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS)

enable_testing()
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE "app_base_zmq" "pthread" ${CONAN_LIBS})
add_test(${PROJECT_NAME} ${PROJECT_NAME})
