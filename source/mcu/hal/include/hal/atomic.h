#pragma once

#include <stdlib.h>
#include <stdbool.h>

#define _HAL_ATOMIC_DECL_BASE(name, type) \
\
    typedef struct { \
        volatile type value; \
    } hal_atomic_##name##_t; \
\
    type hal_atomic_##name##_load(const hal_atomic_##name##_t *self); \
    void hal_atomic_##name##_store(hal_atomic_##name##_t *self, type new_value); \
\
    void hal_atomic_##name##_add(hal_atomic_##name##_t *self, type arg);

#define _HAL_ATOMIC_DECL_UNSIGNED(name, type) \
    _HAL_ATOMIC_DECL_BASE(name, type) \
\
    /** Subtract `arg` from `self` and write result to `self`. On success (no underflow) returns 0. \
        If `arg` is greater than `self` then `self` is set to 0 and difference between `arg` and `self` is returned. */ \
    type hal_atomic_##name##_sub_checked(hal_atomic_##name##_t *self, type arg);

_HAL_ATOMIC_DECL_UNSIGNED(size, size_t)
