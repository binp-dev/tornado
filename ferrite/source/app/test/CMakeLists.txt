cmake_minimum_required(VERSION 3.16)

project("app_base_test")

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
