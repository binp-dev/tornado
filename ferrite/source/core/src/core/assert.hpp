#pragma once

#include "panic.hpp"

#define core_assert(value) \
    do { \
        if (!(value)) [[unlikely]] { \
            core_panic("Assertion failed: {} is false", #value); \
        } \
    } while (0)

#define core_assert_eq(left, right) \
    do { \
        if (!((left) == (right))) [[unlikely]] { \
            core_panic("Assertion failed: expected {} == {}, but got {} != {}", #left, #right, (left), (right)); \
        } \
    } while (0)

#define core_assert_ne(left, right) \
    do { \
        if (!((left) != (right))) [[unlikely]] { \
            core_panic("Assertion failed: expected {} != {}, but got {} == {}", #left, #right, (left), (right)); \
        } \
    } while (0)
