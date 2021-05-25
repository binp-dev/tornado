# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 3.1.1)

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

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})


project(rpmsg_lite_str_echo_rtos_imxcm7)

set(MCUX_SDK_PROJECT_NAME rpmsg_lite_str_echo_rtos_imxcm7.elf)

include(${ProjDirPath}/flags.cmake)

include(${ProjDirPath}/config.cmake)

add_executable(${MCUX_SDK_PROJECT_NAME} 
    "${ProjDirPath}/src/main_remote.c"
    "${ProjDirPath}/src/FreeRTOSConfig.h"
    "${ProjDirPath}/src/pin_mux.c"
    "${ProjDirPath}/src/pin_mux.h"
    "${ProjDirPath}/src/rpmsg_config.h"
    "${ProjDirPath}/src/rsc_table.c"
    "${ProjDirPath}/src/rsc_table.h"
    "${ProjDirPath}/src/remoteproc.h"
    "${ProjDirPath}/src/board.c"
    "${ProjDirPath}/src/board.h"
    "${ProjDirPath}/src/clock_config.c"
    "${ProjDirPath}/src/clock_config.h"
    "${ProjDirPath}/src/fsl_iomuxc.h"
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${ProjDirPath}/src
)

set(CMAKE_MODULE_PATH
    $ENV{FREERTOS_DIR}/middleware/multicore
    $ENV{FREERTOS_DIR}/rtos/freertos
    $ENV{FREERTOS_DIR}/devices/MIMX8MN6/drivers
    $ENV{FREERTOS_DIR}/devices/MIMX8MN6
    $ENV{FREERTOS_DIR}/devices/MIMX8MN6/utilities
    $ENV{FREERTOS_DIR}/components/uart
    $ENV{FREERTOS_DIR}/components/serial_manager
    $ENV{FREERTOS_DIR}/components/lists
    $ENV{FREERTOS_DIR}/CMSIS/Include
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
include(CMSIS_Include_core_cm7_MIMX8MN6)
include(CMSIS_Include_common_MIMX8MN6)
include(CMSIS_Include_dsp_MIMX8MN6)
include(utilities_misc_utilities_MIMX8MN6)
include(device_MIMX8MN6_system_MIMX8MN6)


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
    ${EXECUTABLE_OUTPUT_PATH}/rpmsg_lite_str_echo_rtos.bin
)


# Link HAL

add_subdirectory("../hal" "hal" EXCLUDE_FROM_ALL)
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE "hal_imx8mn")

# Workaround to pass include directories to HAL
get_target_property(MCUX_SDK_INCLUDE_DIRECTORIES ${MCUX_SDK_PROJECT_NAME} INCLUDE_DIRECTORIES)
target_include_directories("hal_imx8mn" PRIVATE ${MCUX_SDK_INCLUDE_DIRECTORIES})
