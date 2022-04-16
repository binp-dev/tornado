#pragma once

#include <type_traits>
#include <limits>
#include <functional>

#include "panic.hpp"
#include "option.hpp"

namespace core {

template <typename Dst, typename Src>
Option<Dst> cast_int(Src src) {
    static_assert(std::is_integral_v<Src>);
    static_assert(std::is_integral_v<Dst>);

    if constexpr (std::is_signed_v<Src> == std::is_signed_v<Dst>) {
        if (src >= std::numeric_limits<Dst>::min() && src <= std::numeric_limits<Dst>::max()) {
            return Some(static_cast<Dst>(src));
        }
    } else if constexpr (std::is_signed_v<Src>) {
        if (src >= static_cast<Src>(0) && static_cast<std::make_unsigned_t<Src>>(src) <= std::numeric_limits<Dst>::max()) {
            return Some(static_cast<Dst>(src));
        }
    } else {
        if (src <= static_cast<std::make_unsigned_t<Dst>>(std::numeric_limits<Dst>::max())) {
            return Some(static_cast<Dst>(src));
        }
    }
    return None();
}

template <typename Dst, typename Src>
Option<std::reference_wrapper<Dst>> downcast(Src &src) {
    static_assert(std::is_base_of_v<Src, Dst>);
    static_assert(std::is_convertible_v<Dst *, Src *>);

    Dst *ptr = dynamic_cast<Dst *>(&src);
    if (ptr != nullptr) {
        return Some(std::ref(*ptr));
    }
    return None();
}

} // namespace core
