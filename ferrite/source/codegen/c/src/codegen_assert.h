#pragma once

#include <stdio.h>

#define codegen_assert(expr) \
    do { \
        if (!(expr)) { \
            printf("Assertion failed: " #expr " is false\n"); \
            printf("File: " __FILE__ ", line: %d\n", __LINE__); \
            fflush(stdout); \
            return 1; \
        } \
    } while (0)

#define codegen_assert_eq(left, right) \
    do { \
        if ((left) != (right)) { \
            printf("Assertion failed: " #left " != " #right "\n"); \
            printf("File: " __FILE__ ", line: %d\n", __LINE__); \
            fflush(stdout); \
            return 1; \
        } \
    } while (0)
