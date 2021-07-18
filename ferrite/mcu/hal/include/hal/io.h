#pragma once

#if defined(HAL_PRINT_UART) && defined(HAL_PRINT_RPMSG)
#error "HAL_PRINT_UART and HAL_PRINT_RPMSG cannot be defined both at once"
#endif

#ifdef HAL_PRINT_UART

#include "debug_console_imx.h"

#define hal_print(...) PRINTF(__VA_ARGS__)

#endif // APP_DEBUG_IO_UART

#ifdef HAL_PRINT_RPMSG

#include <stdio.h>
#include <stdint.h>
#include "rpmsg.h"

void hal_io_rpmsg_init(hal_rpmsg_channel *channel);

hal_rpmsg_channel *__hal_io_rpmsg_channel();
uint8_t *__hal_io_rpmsg_alloc_buffer(size_t *size);
void __hal_io_rpmsg_send_buffer(uint8_t *buffer, size_t size);

#define hal_print(...) do { \
    if (__hal_io_rpmsg_channel() != NULL) { \
        size_t __size = 0; \
        uint8_t *__buffer = __hal_io_rpmsg_alloc_buffer(&__size); \
        const size_t __text_size = snprintf((char*)__buffer + 1, __size - 2, __VA_ARGS__); \
        __hal_io_rpmsg_send_buffer(__buffer, __text_size + 2); \
    } \
} while(0)

#endif
