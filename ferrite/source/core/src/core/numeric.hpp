#pragma once

#include <type_traits>
#include <cstdlib>

namespace core {

// Got from https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
template <typename T>
T next_power_of_two(T v) {
    static_assert(std::is_integral_v<T>);
    v--;
    for (size_t i = 1; i < sizeof(T) * 8; i = (i << 1)) {
        v |= (v >> i);
    }
    v++;
    return v;
}

} // namespace core
