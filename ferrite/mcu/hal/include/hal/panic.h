#pragma once

#include <stdbool.h>
#include "io.h"

__attribute__((noreturn))
void __hal_panic();

extern bool __hal_panicked;

#define hal_panic() do { \
    bool __panicked = __hal_panicked; \
    __hal_panicked = true; \
    if (!__panicked) { \
        hal_error(0xFF, "Panic in %s at %s:%d", __FUNCTION__, __FILE__, __LINE__); \
    } \
    __hal_panic(); \
} while(0)
