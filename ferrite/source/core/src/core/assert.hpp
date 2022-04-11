#pragma once

#include "panic.hpp"

#include <sstream>

#define core_assert(value) do { \
    if (__builtin_expect(!(value), 0)) { \
        std::stringstream ss; \
        ss << "Assertion failed: " << #value << " is false"; \
        panic(ss.str()); \
    } \
} while(0)

#define core_assert_eq(left, right) do { \
    if (__builtin_expect(!((left) == (right)), 0)) { \
        std::stringstream ss; \
        ss << "Assertion failed: expected " << #left << " == " << #right \
            << ", but got " << (left) << " != " << (right); \
        panic(ss.str()); \
    } \
} while(0)

#define core_assert_ne(left, right) do { \
    if (__builtin_expect(!((left) != (right)), 0)) { \
        std::stringstream ss; \
        ss << "Assertion failed: expected " << #left << " != " << #right \
            << ", but got " << (left) << " == " << (right); \
        panic(ss.str()); \
    } \
} while(0)
