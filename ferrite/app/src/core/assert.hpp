#pragma once

#include "panic.hpp"

inline void assert_true(bool value) {
    if (!value) {
        panic("Assertion failed");
    }
}

inline void assert_false(bool value) {
    if (value) {
        panic("Assertion failed");
    }
}

template <typename T, typename U>
void assert_eq(const T &left, const U &right) {
    if (!(left == right)) {
        panic("Assertion failed");
    }
}

template <typename T, typename U>
void assert_ne(const T &left, const U &right) {
    if (!(left != right)) {
        panic("Assertion failed");
    }
}
