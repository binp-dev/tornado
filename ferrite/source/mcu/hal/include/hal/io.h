#pragma once

#if defined(HAL_PRINT_UART) && defined(HAL_PRINT_RPMSG)
#error "HAL_PRINT_UART and HAL_PRINT_RPMSG cannot be defined both at once"
#endif

#ifdef HAL_PRINT_UART

#include <stdio.h>

#if defined(HAL_IMX7)
#include "debug_console_imx.h"
#elif defined(HAL_IMX8MN)
#include "fsl_debug_console.h"
#include "fsl_uart.h"
#else
#error "Unknown target"
#endif

void hal_io_uart_init(uint32_t index);

#define __HAL_IO_BUFFER_SIZE 0x100
extern char __hal_io_buffer[__HAL_IO_BUFFER_SIZE];

#define hal_print(...) \
    do { \
        snprintf(__hal_io_buffer, __HAL_IO_BUFFER_SIZE, __VA_ARGS__); \
        __hal_io_buffer[__HAL_IO_BUFFER_SIZE - 1] = '\0'; \
        PRINTF("%s\r\n", __hal_io_buffer); \
    } while (0)

#define hal_error(code, ...) \
    do { \
        PRINTF("Error (%d): ", (int)(code)); \
        hal_print(__VA_ARGS__); \
    } while (0)

#endif

#ifdef HAL_PRINT_RPMSG

#include <stdio.h>
#include <stdint.h>
#include "rpmsg.h"

#include <ipp.h>

void hal_io_rpmsg_init(hal_rpmsg_channel *channel);

hal_rpmsg_channel *__hal_io_rpmsg_channel();
uint8_t *__hal_io_rpmsg_alloc_buffer(size_t *size);
void __hal_io_rpmsg_send_debug_message(uint8_t unused, uint8_t *buffer);
void __hal_io_rpmsg_send_error_message(uint8_t code, uint8_t *buffer);

#define __hal_send_message(union_field, function_to_send, code_or_unused, ...) \
    do { \
        if (__hal_io_rpmsg_channel() != NULL) { \
            size_t __size = 0; \
            uint8_t *__buffer = __hal_io_rpmsg_alloc_buffer(&__size); \
            IppString *__message = &(((IppMcuMsg *)__buffer)->union_field.message); \
            const size_t __offset = (size_t)((uint8_t *)(__message->data) - __buffer); \
            const size_t __text_size = snprintf(__message->data, __size - __offset, __VA_ARGS__); \
            __message->len = __text_size; \
            function_to_send((code_or_unused), __buffer); \
        } \
    } while (0)

#define hal_print(...) __hal_send_message(debug, __hal_io_rpmsg_send_debug_message, 0, __VA_ARGS__)
#define hal_error(code, ...) __hal_send_message(error, __hal_io_rpmsg_send_error_message, (code), __VA_ARGS__)

#endif
