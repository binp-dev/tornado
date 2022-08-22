#pragma once

#include <stdio.h>

#include <epicsExit.h>

#define fer_epics_assert(expr) \
    do { \
        if (__builtin_expect(!(expr), 0)) { \
            printf("Assertion failed: %s is false\n", #expr); \
            epicsExit(2); \
        } \
    } while (0)
