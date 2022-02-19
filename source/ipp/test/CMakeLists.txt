cmake_minimum_required(VERSION 3.16)

project("ipp_test")

add_subdirectory("${FERRITE}/app/cmake/config" "config")
add_subdirectory(".." "ipp")

set(SRC
    "../src/ipp_test.cpp"
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS)

enable_testing()
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE "core" "ipp_cpp" ${CONAN_LIBS})
add_test(${PROJECT_NAME} ${PROJECT_NAME})
