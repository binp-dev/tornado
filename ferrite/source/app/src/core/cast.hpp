#pragma once

#include <type_traits>
#include <limits>
#include <optional>

template <typename Dst, typename Src>
std::optional<Dst> safe_cast(Src src) {
    static_assert(std::is_integral_v<Src> && std::is_integral_v<Dst>);
    if constexpr (std::is_signed_v<Src> == std::is_signed_v<Dst>) {
        if (src >= std::numeric_limits<Dst>::min() && src <= std::numeric_limits<Dst>::max()) {
            return static_cast<Dst>(src);
        }
    } else if constexpr (std::is_signed_v<Src>) {
        if (src >= static_cast<Src>(0) && static_cast<std::make_unsigned_t<Src>>(src) <= std::numeric_limits<Dst>::max()) {
            return static_cast<Dst>(src);
        }
    } else {
        if (src <= static_cast<std::make_unsigned_t<Dst>>(std::numeric_limits<Dst>::max())) {
            return static_cast<Dst>(src);
        }
    }
    return std::nullopt;
}
