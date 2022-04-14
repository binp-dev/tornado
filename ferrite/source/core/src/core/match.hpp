#pragma once

namespace core {

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

} // namespace core
