cmake_minimum_required(VERSION 3.16)

project("ipp")

include_directories(
    "src"
)

set(SRC
    "src/ipp.c"
)
set(SRC_CPP
    ${SRC}
    "src/ipp.cpp"
)
set(SRC_UNITTEST
    ${SRC_CPP}
    "src/test.cpp"
)

set(LIB ${PROJECT_NAME})
add_library(${LIB} OBJECT ${SRC})

set(LIB_CPP "${PROJECT_NAME}_cpp")
add_library(${LIB_CPP} OBJECT ${SRC_CPP})

enable_testing()
set(UNITTEST "${PROJECT_NAME}_unittest")
add_executable(${UNITTEST} ${SRC_UNITTEST})
target_compile_definitions(${UNITTEST} PRIVATE "UNITTEST")
set_property(TARGET ${UNITTEST} PROPERTY CXX_STANDARD 17)
target_link_libraries(${UNITTEST} PRIVATE "gtest" "pthread")
add_test(${UNITTEST} ${UNITTEST})
