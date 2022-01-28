cmake_minimum_required(VERSION 3.16)

project("app")

set(CMAKE_C_FLAGS_DEBUG "-g -rdynamic -funwind-tables")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror=return-type")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -fno-exceptions")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

include_directories("src")

if(NOT DEFINED APP_LIB_DIR)
    message(FATAL_ERROR "APP_LIB_DIR is not defined")
endif()
include_directories("${APP_LIB_DIR}/src")
add_subdirectory(${APP_LIB_DIR} "app_lib")

if(NOT DEFINED IPP_GEN_DIR)
    message(FATAL_ERROR "IPP_GEN_DIR is not defined")
endif()
include_directories("${IPP_GEN_DIR}/include")
add_subdirectory(${IPP_GEN_DIR} "ipp")


set(BASE_SRC
    "src/device.hpp"
    "src/device.cpp"
    "src/handlers.hpp"
    "src/framework.cpp"
)
set(MAIN_SRC
    ${BASE_SRC}
)
set(FAKEDEV_SRC
    ${BASE_SRC}
)

if(DEFINED APP_FAKEDEV)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS)
endif()

set(BASE_DEFS)
set(MAIN_DEFS
    ${BASE_DEFS}
)
set(FAKEDEV_DEFS
    ${BASE_DEFS}
    "FAKEDEV"
)

set(BASE_LIBS
    "ipp_cpp"
)
set(MAIN_LIBS
    ${BASE_LIBS}
    "app_lib"
)
set(FAKEDEV_LIBS
    ${BASE_LIBS}
    "app_lib_fakedev"
    ${CONAN_LIBS}
)

if(DEFINED APP_MAIN)
    set(MAIN ${PROJECT_NAME})
    add_library(${MAIN} SHARED ${MAIN_SRC})
    target_compile_definitions(${MAIN} PRIVATE ${MAIN_DEFS})
    target_link_libraries(${MAIN} PRIVATE ${MAIN_LIBS})
endif()

if(DEFINED APP_FAKEDEV)
    set(FAKEDEV "${PROJECT_NAME}_fakedev")
    add_library(${FAKEDEV} SHARED ${FAKEDEV_SRC})
    target_compile_definitions(${FAKEDEV} PRIVATE ${FAKEDEV_DEFS})
    target_link_libraries(${FAKEDEV} PRIVATE ${FAKEDEV_LIBS})
endif()
