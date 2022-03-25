#pragma once

#include <string>
#include <iostream>
#include <type_traits>
#include <cstdint>

template <typename T, typename = void>
struct Display : public std::false_type {};

template <typename T>
struct Display<T, std::void_t<decltype(std::declval<std::ostream &>() << std::declval<T>())>> : public std::true_type {};

template <typename T>
constexpr bool is_display_v = Display<T>::value;
