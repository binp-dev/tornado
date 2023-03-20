SET(CMAKE_ASM_FLAGS " \
    ${CMAKE_ASM_FLAGS} \
    -DNDEBUG \
    -D__STARTUP_CLEAR_BSS \
    -D__STARTUP_INITIALIZE_NONCACHEDATA \
    -mcpu=cortex-m7 \
    -Wall \
    -Wno-address-of-packed-member \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -mthumb \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -ffreestanding \
    -fno-builtin \
    -mapcs \
    -std=gnu99 \
")

SET(CMAKE_C_FLAGS " \
    ${CMAKE_C_FLAGS} \
    -DNDEBUG \
    -DCPU_MIMX8MN6DVTJZ \
    -DSDK_DELAY_USE_DWT \
    -DMCUXPRESSO_SDK \
    -DFSL_RTOS_FREE_RTOS \
    -DSERIAL_PORT_TYPE_UART=1 \
    -Os \
    -mcpu=cortex-m7 \
    -Wall \
    -Wno-address-of-packed-member \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -mthumb \
    -MMD \
    -MP \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -ffreestanding \
    -fno-builtin \
    -mapcs \
    -std=gnu99 \
")

SET(CMAKE_CXX_FLAGS " \
    ${CMAKE_CXX_FLAGS} \
    -DNDEBUG \
    -DCPU_MIMX8MN6DVTJZ \
    -DMCUXPRESSO_SDK \
    -DSERIAL_PORT_TYPE_UART=1 \
    -Os \
    -mcpu=cortex-m7 \
    -Wall \
    -Wno-address-of-packed-member \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -mthumb \
    -MMD \
    -MP \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -ffreestanding \
    -fno-builtin \
    -mapcs \
    -fno-rtti \
    -fno-exceptions \
")

SET(CMAKE_EXE_LINKER_FLAGS " \
    ${CMAKE_EXE_LINKER_FLAGS} \
    -mcpu=cortex-m7 \
    -Wall \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -Wl,--print-memory-usage \
    --specs=nano.specs \
    --specs=nosys.specs \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -ffreestanding \
    -fno-builtin \
    -mthumb \
    -mapcs \
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
    -T${ProjDirPath}/linker/MIMX8MN6xxxxx_cm7_ram.ld -static \
")
