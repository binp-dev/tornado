#pragma once

#include "panic.hpp"

#include <sstream>

#define assert_true(value) do { \
    if (__builtin_expect(!(value), 0)) { \
        std::stringstream ss; \
        ss << "Assertion failed: " << #value << " is false"; \
        panic(ss.str()); \
    } \
} while(0)

#define assert_false(value) do { \
    if (__builtin_expect((value), 0)) { \
        std::stringstream ss; \
        ss << "Assertion failed: " << #value << " is true"; \
        panic(ss.str()); \
    } \
} while(0)

#define assert_eq(left, right) do { \
    if (__builtin_expect(!((left) == (right)), 0)) { \
        std::stringstream ss; \
        ss << "Assertion failed: expected " << #left << " == " << #right \
            << ", but got " << (left) << " != " << (right); \
        panic(ss.str()); \
    } \
} while(0)

#define assert_ne(left, right) do { \
    if (__builtin_expect(!((left) != (right)), 0)) { \
        std::stringstream ss; \
        ss << "Assertion failed: expected " << #left << " != " << #right \
            << ", but got " << (left) << " == " << (right); \
        panic(ss.str()); \
    } \
} while(0)
