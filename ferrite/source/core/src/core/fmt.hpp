#pragma once

#include <string>
#include <iostream>
#include <type_traits>

template <typename T, typename = void>
struct Display : std::false_type {};

template <typename T>
constexpr bool display_v = Display<T>::value;

template <typename T>
struct Display<T, std::void_t<decltype(std::declval<std::ostream &>() << std::declval<T>())>> : std::true_type {};
