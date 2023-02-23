#pragma once

#include <stdint.h>
#include "io.h"

__attribute__((noreturn)) void __ustd_panic();

extern uint8_t __ustd_panicked;

#define hal_panic() \
    do { \
        if (__ustd_panicked == 0) { \
            hal_error(0xFF, "Panic in %s at %s:%d", __FUNCTION__, __FILE__, __LINE__); \
        } \
        __ustd_panic(); \
    } while (0)

#define hal_unreachable hal_panic
