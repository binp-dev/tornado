INCLUDE(CMakeForceCompiler)

# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

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

# DEBUG LINK FILE
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -T$ENV{FREERTOS_DIR}/platform/devices/MCIMX7D/linker/gcc/MCIMX7D_M4_tcm.ld  -static")

# RELEASE LINK FILE
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -T$ENV{FREERTOS_DIR}/platform/devices/MCIMX7D/linker/gcc/MCIMX7D_M4_tcm.ld  -static")

# DEBUG ASM FLAGS
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} -g  -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  -mthumb  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# DEBUG C FLAGS
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g  -O0  -fno-strict-aliasing  -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  -mthumb  -MMD  -MP  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# DEBUG LD FLAGS
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g  -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  --specs=nano.specs  -lm  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -Os  -mthumb  -mapcs  -Xlinker --gc-sections  -Xlinker -static  -Xlinker -z  -Xlinker muldefs")

# RELEASE ASM FLAGS
SET(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE} -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  -mthumb  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# RELEASE C FLAGS
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os  -fno-strict-aliasing  -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  -mthumb  -MMD  -MP  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# RELEASE LD FLAGS
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  --specs=nano.specs  -lm  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -Os  -mthumb  -mapcs  -Xlinker --gc-sections  -Xlinker -static  -Xlinker -z  -Xlinker muldefs")

# ASM MACRO
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG}  -D__DEBUG")

# C MACRO
SET(CMAKE_C_FLAGS_COMMON "-DAPP_DEBUG_IO_RPMSG")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  ${CMAKE_C_FLAGS_COMMON}")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -D__DEBUG")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -DCPU_MCIMX7D_M4")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -DAPP_LOG_LEVEL=4") # DEBUG
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  ${CMAKE_C_FLAGS_COMMON}")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -D__NDEBUG")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DCPU_MCIMX7D_M4")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DAPP_LOG_LEVEL=3") # INFO

# CXX MACRO

# INCLUDE_DIRECTORIES
INCLUDE_DIRECTORIES($ENV{COMMON_DIR})
INCLUDE_DIRECTORIES(${ProjDirPath}/src)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/examples/var-som-mx7_m4)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/portable/GCC/ARM_CM4F)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/platform/CMSIS/Include)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/platform/devices)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/platform/devices/MCIMX7D/include)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/platform/devices/MCIMX7D/startup)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/platform/drivers/inc)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/platform/utilities/inc)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/examples/var-som-mx7_m4)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/middleware/multicore/open-amp)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/imx7d_m4)
INCLUDE_DIRECTORIES($ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/env/freertos)

# ADD_EXECUTABLE
ADD_EXECUTABLE(psc_m4 
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/portable/GCC/ARM_CM4F/portmacro.h"
    "$ENV{FREERTOS_DIR}/platform/devices/MCIMX7D/startup/gcc/startup_MCIMX7D_M4.S"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/portable/MemMang/heap_2.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/rpmsg/rpmsg_rtos.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/imx7d_m4/platform.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/imx7d_m4/rpmsg_platform_porting.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/rpmsg/rpmsg.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/rpmsg/rpmsg_ext.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/common/hil/hil.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/common/llist/llist.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/common/shm/sh_mem.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/config/config.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/env/env.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/rpmsg/rpmsg_core.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/virtio/virtio.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/virtio/virtio_ring.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/virtio/virtqueue.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/env/freertos/rpmsg_porting.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/croutine.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/event_groups.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/FreeRTOS.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/list.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/mpu_wrappers.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/portable.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/projdefs.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/queue.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/semphr.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/StackMacros.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/task.h"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/include/timers.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/ccm_analog_imx7d.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/ccm_imx7d.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/lmem.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/rdc.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/rdc_defs_imx7d.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/wdog_imx.h"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/rpmsg/rpmsg_rtos.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/imx7d_m4/platform.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/imx7d_m4/platform_info.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/common/hil/hil.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/common/llist/llist.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/common/shm/sh_mem.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/config/config.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/rpmsg/remote_device.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/rpmsg/rpmsg.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/rpmsg/rpmsg_ext.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/rpmsg/rpmsg_core.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/virtio/virtio.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/virtio/virtqueue.c"
    "$ENV{FREERTOS_DIR}/middleware/multicore/open-amp/porting/env/freertos/rpmsg_porting.c"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/croutine.c"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/event_groups.c"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/list.c"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/queue.c"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/tasks.c"
    "$ENV{FREERTOS_DIR}/rtos/FreeRTOS/Source/timers.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/ccm_analog_imx7d.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/ccm_imx7d.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/lmem.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/rdc.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/wdog_imx.c"
    "$ENV{FREERTOS_DIR}/platform/utilities/src/debug_console_imx.c"
    "$ENV{FREERTOS_DIR}/platform/utilities/inc/debug_console_imx.h"
    "$ENV{FREERTOS_DIR}/platform/utilities/src/print_scan.c"
    "$ENV{FREERTOS_DIR}/platform/utilities/src/print_scan.h"
    "$ENV{FREERTOS_DIR}/platform/devices/MCIMX7D/startup/system_MCIMX7D_M4.c"
    "$ENV{FREERTOS_DIR}/platform/devices/MCIMX7D/startup/system_MCIMX7D_M4.h"
    "$ENV{FREERTOS_DIR}/examples/var-som-mx7_m4/pin_mux.c"
    "$ENV{FREERTOS_DIR}/examples/var-som-mx7_m4/pin_mux.h"
    "$ENV{FREERTOS_DIR}/examples/var-som-mx7_m4/board.c"
    "$ENV{FREERTOS_DIR}/examples/var-som-mx7_m4/board.h"
    "$ENV{FREERTOS_DIR}/examples/var-som-mx7_m4/clock_freq.c"
    "$ENV{FREERTOS_DIR}/examples/var-som-mx7_m4/clock_freq.h"
    "$ENV{FREERTOS_DIR}/examples/var-som-mx7_m4/gpio_pins.c"
    "$ENV{FREERTOS_DIR}/examples/var-som-mx7_m4/gpio_pins.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/uart_imx.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/uart_imx.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/mu_imx.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/mu_imx.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/ecspi.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/ecspi.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/gpio_imx.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/gpio_imx.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/gpt.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/gpt.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/rdc_semaphore.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/rdc_semaphore.h"
    "$ENV{FREERTOS_DIR}/platform/drivers/src/flexcan.c"
    "$ENV{FREERTOS_DIR}/platform/drivers/inc/flexcan.h"
    "${ProjDirPath}/src/FreeRTOSConfig.h"
    "${ProjDirPath}/src/main.c"
)
SET_TARGET_PROPERTIES(psc_m4 PROPERTIES OUTPUT_NAME "m4image.elf")

TARGET_LINK_LIBRARIES(psc_m4 -Wl,--start-group)
# LIBRARIES
IF(CMAKE_BUILD_TYPE MATCHES Debug)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
ENDIF()

# SYSTEM LIBRARIES
TARGET_LINK_LIBRARIES(psc_m4 m)
TARGET_LINK_LIBRARIES(psc_m4 c)
TARGET_LINK_LIBRARIES(psc_m4 gcc)
TARGET_LINK_LIBRARIES(psc_m4 nosys)
TARGET_LINK_LIBRARIES(psc_m4 -Wl,--end-group)

# MAP FILE
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}  -Xlinker -Map=debug/m4image.map")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}  -Xlinker -Map=release/m4image.map")

# BIN AND HEX
ADD_CUSTOM_COMMAND(TARGET psc_m4 POST_BUILD COMMAND ${CMAKE_OBJCOPY} -Oihex ${EXECUTABLE_OUTPUT_PATH}/m4image.elf ${EXECUTABLE_OUTPUT_PATH}/m4image.hex)
ADD_CUSTOM_COMMAND(TARGET psc_m4 POST_BUILD COMMAND ${CMAKE_OBJCOPY} -Obinary ${EXECUTABLE_OUTPUT_PATH}/m4image.elf ${EXECUTABLE_OUTPUT_PATH}/m4image.bin)

# Link common code
set(HAL_TARGET "hal_imx7")
add_definitions("-DHAL_IMX7")
add_subdirectory("../common" "common" EXCLUDE_FROM_ALL)
include_directories("../common/include")
target_link_libraries(psc_m4 PRIVATE
    "common"
    "ipp"
    ${HAL_TARGET}
)
