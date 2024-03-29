#pragma once

#include <hal/panic.h>
#include <hal/defs.h>

#define hal_assert(expr) \
    do { \
        if (!(expr)) { \
            hal_print("Assertion failed: %s", #expr); \
            hal_panic(); \
        } \
    } while (0)

#define hal_assert_retcode(expr) \
    do { \
        hal_retcode code = (expr); \
        if (code != HAL_SUCCESS) { \
            hal_print("Bad retcode: %s returned %s", #expr, hal_retcode_str(code)); \
            hal_panic(); \
        } \
    } while (0)
