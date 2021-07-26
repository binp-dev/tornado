#pragma once

#if defined(HAL_PRINT_UART) && defined(HAL_PRINT_RPMSG)
#error "HAL_PRINT_UART and HAL_PRINT_RPMSG cannot be defined both at once"
#endif

#ifdef HAL_PRINT_UART

#include "debug_console_imx.h"

#define hal_debug(...) do { \
    PRINTF(__VA_ARGS__); \
    PRINTF("\r\n"); \
} while(0)

#define hal_error(code, ...) do { \
    PRINTF("Error (%d): ", (int)(code)); \
    PRINTF(__VA_ARGS__); \
    PRINTF("\r\n"); \
} while(0)

#endif // APP_DEBUG_IO_UART

#ifdef HAL_PRINT_RPMSG

#include <stdio.h>
#include <stdint.h>
#include "rpmsg.h"

void hal_io_rpmsg_init(hal_rpmsg_channel *channel);

hal_rpmsg_channel *__hal_io_rpmsg_channel();
uint8_t *__hal_io_rpmsg_alloc_buffer(size_t *size);
void __hal_io_rpmsg_send_debug_buffer(uint8_t *buffer, size_t size);
void __hal_io_rpmsg_send_error_buffer(uint8_t code, uint8_t *buffer, size_t size);

#define hal_debug(...) do { \
    if (__hal_io_rpmsg_channel() != NULL) { \
        size_t __size = 0; \
        uint8_t *__buffer = __hal_io_rpmsg_alloc_buffer(&__size); \
        const size_t __text_size = snprintf((char*)__buffer + 1, __size - 2, __VA_ARGS__); \
        __hal_io_rpmsg_send_debug_buffer(__buffer, __text_size + 2); \
    } \
} while(0)

#define hal_error(code, ...) do { \
    if (__hal_io_rpmsg_channel() != NULL) { \
        size_t __size = 0; \
        uint8_t *__buffer = __hal_io_rpmsg_alloc_buffer(&__size); \
        const size_t __text_size = snprintf((char*)__buffer + 2, __size - 3, __VA_ARGS__); \
        __hal_io_rpmsg_send_error_buffer((code), __buffer, __text_size + 3); \
    } \
} while(0)

#endif
