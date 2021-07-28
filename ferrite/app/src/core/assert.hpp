#pragma once

#include "panic.hpp"

inline void assert_true(bool value) {
    if (!__builtin_expect(value, 1)) {
        panic("Assertion failed");
    }
}

inline void assert_false(bool value) {
    if (__builtin_expect(value, 0)) {
        panic("Assertion failed");
    }
}

template <typename T, typename U>
void assert_eq(const T &left, const U &right) {
    if (!__builtin_expect(left == right, 1)) {
        panic("Assertion failed");
    }
}

template <typename T, typename U>
void assert_ne(const T &left, const U &right) {
    if (!__builtin_expect(left != right, 0)) {
        panic("Assertion failed");
    }
}
