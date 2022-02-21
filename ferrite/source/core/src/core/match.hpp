#pragma once

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

// TODO: Remove with C++20
template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
