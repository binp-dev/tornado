#pragma once

#include <fsl_debug_console.h>

#define hal_print(...) \
    do { \
        snprintf(__ustd_io_buffer, __USTD_IO_BUFFER_SIZE, __VA_ARGS__); \
        __ustd_print_buffer(); \
        PRINTF("\r\n"); \
    } while (0)
