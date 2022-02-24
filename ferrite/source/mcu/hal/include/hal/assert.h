#pragma once

#include "panic.h"
#include "log.h"

#define hal_assert(expr) \
    do { \
        if (!(expr)) { \
            hal_log_error("Assertion failed: %s", #expr); \
            hal_panic(); \
        } \
    } while (0)
