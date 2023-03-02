#pragma once

#include <stdio.h>
#include <stdint.h>

#if defined(HAL_IMX7)
#include "debug_console_imx.h"
#elif defined(HAL_IMX8MN)
#include "fsl_debug_console.h"
#include "fsl_uart.h"
#else
#error "Unknown target"
#endif

void hal_io_init();
void hal_io_uart_init(uint32_t index);

#define __USTD_IO_BUFFER_SIZE 0x100
extern char __ustd_io_buffer[];

void __ustd_print_buffer();

#define hal_print(...) \
    do { \
        snprintf(__ustd_io_buffer, __USTD_IO_BUFFER_SIZE, __VA_ARGS__); \
        __ustd_print_buffer(); \
        PRINTF("\r\n"); \
    } while (0)

#define hal_error(code, ...) \
    do { \
        PRINTF("Error (%d): ", (int)(code)); \
        hal_print(__VA_ARGS__); \
    } while (0)