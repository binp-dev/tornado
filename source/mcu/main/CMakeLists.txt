# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 3.10.0)

# THE VERSION NUMBER
SET (Tutorial_VERSION_MAJOR 1)
SET (Tutorial_VERSION_MINOR 0)

# ENABLE ASM
ENABLE_LANGUAGE(ASM)

SET(CMAKE_STATIC_LIBRARY_PREFIX)
SET(CMAKE_STATIC_LIBRARY_SUFFIX)

SET(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
SET(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

# CURRENT DIRECTORY
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

set(FREERTOS_DIR $ENV{FREERTOS_DIR})

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})


project(m7image)

set(MCUX_SDK_PROJECT_NAME m7image.elf)

include(${ProjDirPath}/flags.cmake)

include(${ProjDirPath}/config.cmake)

set(SRC
    "${ProjDirPath}/src/main.c"
    
    "${ProjDirPath}/src/device/FreeRTOSConfig.h"
    "${ProjDirPath}/src/device/freertos_impl.c"
    "${ProjDirPath}/src/device/rpmsg_config.h"
    "${ProjDirPath}/src/device/rsc_table.c"
    "${ProjDirPath}/src/device/rsc_table.h"
    "${ProjDirPath}/src/device/remoteproc.h"
    "${ProjDirPath}/src/device/board.c"
    "${ProjDirPath}/src/device/board.h"
    "${ProjDirPath}/src/device/clock_config.c"
    "${ProjDirPath}/src/device/clock_config.h"

    "${ProjDirPath}/src/utils/crc.c"
    "${ProjDirPath}/src/utils/crc.h"

    "${ProjDirPath}/src/drivers/skifio.c"
    "${ProjDirPath}/src/drivers/skifio.h"
    "${ProjDirPath}/src/drivers/sync.c"
    "${ProjDirPath}/src/drivers/sync.h"
    "${ProjDirPath}/src/drivers/wf.c"
    "${ProjDirPath}/src/drivers/wf.h"
)

add_executable(${MCUX_SDK_PROJECT_NAME} ${SRC})

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    "${ProjDirPath}/src"
    "${ProjDirPath}/src/device"
)

set(CMAKE_MODULE_PATH
    ${FREERTOS_DIR}/middleware/multicore
    ${FREERTOS_DIR}/rtos/freertos
    ${FREERTOS_DIR}/devices/MIMX8MN6/drivers
    ${FREERTOS_DIR}/devices/MIMX8MN6
    ${FREERTOS_DIR}/devices/MIMX8MN6/utilities
    ${FREERTOS_DIR}/components/uart
    ${FREERTOS_DIR}/components/serial_manager
    ${FREERTOS_DIR}/components/lists
    ${FREERTOS_DIR}/CMSIS/Core/Include
)

# include modules
include(middleware_multicore_rpmsg_lite_imx8mn_m7_freertos_MIMX8MN6)
include(middleware_freertos-kernel_heap_4_MIMX8MN6)
include(driver_clock_MIMX8MN6)
include(driver_mu_MIMX8MN6)
include(middleware_freertos-kernel_MIMX8MN6)
include(driver_common_MIMX8MN6)
include(driver_rdc_MIMX8MN6)
include(device_MIMX8MN6_CMSIS_MIMX8MN6)
include(utility_debug_console_MIMX8MN6)
include(component_iuart_adapter_MIMX8MN6)
include(component_serial_manager_MIMX8MN6)
include(component_lists_MIMX8MN6)
include(component_serial_manager_uart_MIMX8MN6)
include(driver_iuart_MIMX8MN6)
include(device_MIMX8MN6_startup_MIMX8MN6)
include(utility_assert_MIMX8MN6)
include(middleware_multicore_rpmsg_lite_MIMX8MN6)
include(middleware_freertos-kernel_extension_MIMX8MN6)
include(CMSIS_Include_core_cm_MIMX8MN6)
include(utilities_misc_utilities_MIMX8MN6)
include(device_MIMX8MN6_system_MIMX8MN6)
include(driver_ecspi_MIMX8MN6)
include(driver_ecspi_freertos_MIMX8MN6)
include(driver_igpio_MIMX8MN6)
include(driver_gpt_MIMX8MN6)

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE
    -Wl,--start-group
    m
    c
    gcc
    nosys
    -Wl,--end-group
)

ADD_CUSTOM_COMMAND(TARGET ${MCUX_SDK_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY} -Obinary
    ${EXECUTABLE_OUTPUT_PATH}/${MCUX_SDK_PROJECT_NAME}
    ${EXECUTABLE_OUTPUT_PATH}/m7image.bin
)

if(NOT DEFINED USER)
    message(FATAL_ERROR "Variable 'USER' is not defined")
endif()

# Generate sync internally
add_definitions("-DGENERATE_SYNC")

# Link HAL
add_definitions("-DHAL_PRINT_UART" "-DHAL_LOG_LEVEL=4")
add_subdirectory("../hal" "hal" EXCLUDE_FROM_ALL)
include_directories("../hal/include")
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PUBLIC "hal")

# Link user lib
target_link_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC "${USER}")
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PUBLIC "user")
add_library("freertos_rust_shim" OBJECT EXCLUDE_FROM_ALL "../ustd/freertos-rust/freertos-rust/src/freertos/shim.c")
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PUBLIC "freertos_rust_shim")

# Workaround to pass include directories to subprojects
get_target_property(MCUX_SDK_INCLUDE_DIRECTORIES ${MCUX_SDK_PROJECT_NAME} INCLUDE_DIRECTORIES)
target_include_directories("hal" PRIVATE ${MCUX_SDK_INCLUDE_DIRECTORIES})
target_include_directories("freertos_rust_shim" PRIVATE ${MCUX_SDK_INCLUDE_DIRECTORIES})
