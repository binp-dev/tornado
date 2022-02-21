#pragma once

#include <string>
#include <iostream>
#include <type_traits>

template <typename T>
struct Display : std::false_type {};

template <>
struct Display<std::string> : std::true_type {};
