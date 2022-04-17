#pragma once

#include <cstdint>
#include <utility>

namespace core {

/// Type which size and alignment are identical to `T` but it can be uninitialized.
/// MaybeUninit<T> is POD for any `T`.
template <typename T>
struct MaybeUninit {
    uint8_t payload[sizeof(T)];

    MaybeUninit() = default;

    const T &assume_init() const {
        return *reinterpret_cast<const T *>(this);
    }
    T &assume_init() {
        return *reinterpret_cast<T *>(this);
    }

    template <typename... Args>
    void init_in_place(Args &&...args) {
        new (&assume_init()) T(std::forward<Args>(args)...);
    }
} __attribute__((aligned(alignof(T))));

} // namespace core
