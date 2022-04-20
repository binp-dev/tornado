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

    "${ProjDirPath}/src/utils/macros.h"
    "${ProjDirPath}/src/utils/crc.c"
    "${ProjDirPath}/src/utils/crc.h"
    "${ProjDirPath}/src/utils/ringbuf.h"
    "${ProjDirPath}/src/utils/ringbuf.inl"

    "${ProjDirPath}/src/drivers/skifio.c"
    "${ProjDirPath}/src/drivers/skifio.h"

    "${ProjDirPath}/src/tasks/stats.c"
    "${ProjDirPath}/src/tasks/stats.h"
    "${ProjDirPath}/src/tasks/control.c"
    "${ProjDirPath}/src/tasks/control.h"
    "${ProjDirPath}/src/tasks/rpmsg.c"
    "${ProjDirPath}/src/tasks/rpmsg.h"
    "${ProjDirPath}/src/tasks/sync.c"
    "${ProjDirPath}/src/tasks/sync.h"
)

add_executable(${MCUX_SDK_PROJECT_NAME} ${SRC})

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    "${ProjDirPath}/src"
    "${ProjDirPath}/src/device"
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
    $ENV{FREERTOS_DIR}/CMSIS/Core/Include
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

if(NOT DEFINED FERRITE)
    message(FATAL_ERROR "Variable 'FERRITE' is not defined")
endif()
if(NOT DEFINED IPP)
    message(FATAL_ERROR "Variable 'IPP' is not defined")
endif()

# Generate sync internally
#add_definitions("-DGENERATE_SYNC")

# Link common code
add_subdirectory("../common" "common" EXCLUDE_FROM_ALL)
include_directories("../common/include")
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE "common")

# Link HAL
set(HAL_TARGET "hal_imx8mn")
add_definitions("-DHAL_IMX8MN" "-DHAL_PRINT_UART" "-DHAL_LOG_LEVEL=4")
add_subdirectory("${FERRITE}/mcu/hal" "hal" EXCLUDE_FROM_ALL)
include_directories("${FERRITE}/mcu/hal/include")
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PUBLIC ${HAL_TARGET})

# Link IPP proto
add_subdirectory(${IPP} "ipp" EXCLUDE_FROM_ALL)
include_directories("${IPP}/include")
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PUBLIC "ipp")

# Workaround to pass include directories to subprojects
get_target_property(MCUX_SDK_INCLUDE_DIRECTORIES ${MCUX_SDK_PROJECT_NAME} INCLUDE_DIRECTORIES)
target_include_directories("common" PRIVATE ${MCUX_SDK_INCLUDE_DIRECTORIES})
target_include_directories(${HAL_TARGET} PRIVATE ${MCUX_SDK_INCLUDE_DIRECTORIES})
target_include_directories("ipp" PRIVATE ${MCUX_SDK_INCLUDE_DIRECTORIES})
