SET(COMMON_FLAGS " \
    -Wall \
    -mcpu=cortex-m7 \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -mthumb \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -ffreestanding \
    -fno-builtin \
    -mapcs \
")

SET(COMMON_LANG_FLAGS " \
    ${COMMON_FLAGS} \
    -DNDEBUG \
    -Wno-address-of-packed-member \
")

SET(CMAKE_ASM_FLAGS " \
    ${CMAKE_ASM_FLAGS} \
    ${COMMON_LANG_FLAGS} \
    -D__STARTUP_CLEAR_BSS \
    -D__STARTUP_INITIALIZE_NONCACHEDATA \
    -std=gnu99 \
")

SET(COMMON_C_FLAGS " \
    ${COMMON_LANG_FLAGS} \
    -DCPU_MIMX8MN6DVTJZ \
    -DMCUXPRESSO_SDK \
    -DSERIAL_PORT_TYPE_UART=1 \
    -Os \
    -MMD \
    -MP \
")

SET(CMAKE_C_FLAGS " \
    ${CMAKE_C_FLAGS} \
    ${COMMON_C_FLAGS} \
    -DSDK_DELAY_USE_DWT \
    -DFSL_RTOS_FREE_RTOS \
    -std=gnu99 \
")

SET(CMAKE_CXX_FLAGS " \
    ${CMAKE_CXX_FLAGS} \
    ${COMMON_C_FLAGS} \
    -fno-rtti \
    -fno-exceptions \
")

SET(CMAKE_EXE_LINKER_FLAGS " \
    ${CMAKE_EXE_LINKER_FLAGS} \
    ${COMMON_FLAGS} \
    -Wl,--print-memory-usage \
    --specs=nano.specs \
    --specs=nosys.specs \
    -Xlinker \
    --gc-sections \
    -Xlinker \
    -static \
    -Xlinker \
    -z \
    -Xlinker \
    muldefs \
    -Xlinker \
    -Map=output.map \
    -Xlinker \
    --defsym=__stack_size__=0x400 \
    -Xlinker \
    --defsym=__heap_size__=0x400 \
    -T${FREERTOS_DIR}/devices/MIMX8MN6/gcc/MIMX8MN6xxxxx_cm7_ram.ld -static \
")
