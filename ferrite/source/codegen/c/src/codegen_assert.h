#pragma once

#include <stdio.h>

#define codegen_assert(expr) \
    do { \
        if (!(expr)) { \
            printf("Assertion failed: " #expr " is false"); \
            return 1; \
        } \
    } while (0)

#define codegen_assert_eq(left, right) \
    do { \
        if ((left) != (right)) { \
            printf("Assertion failed: " #left " != " #right); \
            return 1; \
        } \
    } while (0)
