cmake_minimum_required(VERSION 3.0)

project("app")

set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions -pedantic")

include_directories("src")

set(COMMON_SRC)
set(MAIN_SRC
    ${COMMON_SRC}
    "src/channel/rpmsg.cpp"
)
set(FAKEDEV_SRC
    ${COMMON_SRC}
    "src/channel/zmq.cpp"
)
set(UNITTEST_SRC
    ${FAKEDEV_SRC}
    "src/channel/zmq_test.cpp"
    "src/unittest.cpp"
)

set(COMMON_DEFS)
set(MAIN_DEFS
    ${COMMON_DEFS}
)
set(FAKEDEV_DEFS
    ${COMMON_DEFS}
    "FAKEDEV"
)
set(UNITTEST_DEFS
    ${FAKEDEV_DEFS}
    "UNITTEST"
)

set(COMMON_LIBS)
set(MAIN_LIBS
    ${COMMON_LIBS}
)
set(FAKEDEV_LIBS
    ${COMMON_LIBS}
    "pthread"
    "zmq"
)
set(UNITTEST_LIBS
    "gtest"
    ${FAKEDEV_LIBS}
)

set(MAIN ${PROJECT_NAME})
add_library(${MAIN} STATIC ${MAIN_SRC})
target_compile_definitions(${MAIN} PRIVATE ${MAIN_DEFS})
target_link_libraries(${MAIN} PRIVATE ${MAIN_LIBS})

set(FAKEDEV "${PROJECT_NAME}_fakedev")
add_library(${FAKEDEV} STATIC ${FAKEDEV_SRC})
target_compile_definitions(${FAKEDEV} PRIVATE ${FAKEDEV_DEFS})
target_link_libraries(${FAKEDEV} PRIVATE ${FAKEDEV_LIBS})

enable_testing()
set(UNITTEST "${PROJECT_NAME}_unittest")
add_executable(${UNITTEST} ${UNITTEST_SRC})
target_compile_definitions(${UNITTEST} PRIVATE ${UNITTEST_DEFS})
target_link_libraries(${UNITTEST} PRIVATE ${UNITTEST_LIBS})
add_test(${UNITTEST} ${UNITTEST})
