cmake_minimum_required(VERSION 3.16)

project("app_lib")

set(CMAKE_C_FLAGS_DEBUG "-g -rdynamic -funwind-tables")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories("src")
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

set(COMMON_SRC
    "src/core/assert.hpp"
    "src/core/cast.hpp"
    "src/core/cast.cpp"
    "src/core/io.hpp"
    "src/core/io.cpp"
    "src/core/lazy_static.hpp"
    "src/core/maybe_uninit.hpp"
    "src/core/mutex.hpp"
    "src/core/panic.hpp"
    "src/core/panic.cpp"
    "src/core/result.hpp"

    "src/record/array.hpp"
    "src/record/base.hpp"
    "src/record/value.hpp"

    "src/channel/base.hpp"
    "src/channel/base.cpp"
    "src/channel/message.hpp"
    "src/channel/message.hxx"
)

set(MAIN_SRC
    ${COMMON_SRC}

    "src/channel/rpmsg.hpp"
    "src/channel/rpmsg.cpp"
)
set(FAKEDEV_SRC
    ${COMMON_SRC}

    "src/channel/zmq.hpp"
    "src/channel/zmq.cpp"
)

set(UNITTEST_SRC
    ${COMMON_SRC}

    "src/channel/zmq.hpp"
    "src/channel/zmq.cpp"
    "src/channel/zmq_test.cpp"

    "src/unittest.cpp"
)

if(DEFINED APP_FAKEDEV OR DEFINED APP_TEST)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS)
endif()

set(COMMON_DEFS)
set(MAIN_DEFS
    ${COMMON_DEFS}
)
set(FAKEDEV_DEFS
    ${COMMON_DEFS}
    "FAKEDEV"
)
set(UNITTEST_DEFS
    ${COMMON_DEFS}
    "UNITTEST"
)

set(COMMON_LIBS)
set(MAIN_LIBS
    ${COMMON_LIBS}
)
set(FAKEDEV_LIBS
    ${COMMON_LIBS}
    ${CONAN_LIBS}
)
set(UNITTEST_LIBS
    "pthread"
    "gtest"
    ${CONAN_LIBS}
)

if(DEFINED APP_MAIN)
    set(MAIN ${PROJECT_NAME})
    add_library(${MAIN} OBJECT ${MAIN_SRC})
    target_compile_definitions(${MAIN} PRIVATE ${MAIN_DEFS})
    target_link_libraries(${MAIN} PRIVATE ${MAIN_LIBS})
endif()

if(DEFINED APP_FAKEDEV)
    set(FAKEDEV "${PROJECT_NAME}_fakedev")
    add_library(${FAKEDEV} OBJECT ${FAKEDEV_SRC})
    target_compile_definitions(${FAKEDEV} PRIVATE ${FAKEDEV_DEFS})
    target_link_libraries(${FAKEDEV} PRIVATE ${FAKEDEV_LIBS})
endif()

if(DEFINED APP_TEST)
    enable_testing()
    set(UNITTEST "${PROJECT_NAME}_unittest")
    add_executable(${UNITTEST} ${UNITTEST_SRC})
    target_compile_definitions(${UNITTEST} PRIVATE ${UNITTEST_DEFS})
    target_link_libraries(${UNITTEST} PRIVATE ${UNITTEST_LIBS})
    add_test(${UNITTEST} ${UNITTEST})
endif()
