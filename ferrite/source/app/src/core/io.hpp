#pragma once

#include <iostream>
#include <type_traits>
#include <sstream>

template <typename T>
struct IsWritable : std::false_type {};

template <>
struct IsWritable<std::string> : std::true_type {};

template <typename T>
constexpr bool is_writable = IsWritable<T>::value;
