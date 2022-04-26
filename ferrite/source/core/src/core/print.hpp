#pragma once

#include <iostream>
#include <concepts>
#include <cstdint>

namespace core {

template <typename T>
concept _StdPrintable = requires(std::ostream &stream, const T &value) {
    { stream << value } -> std::same_as<std::ostream &>;
};

template <typename T>
struct Print {};

template <typename T>
concept Printable = requires(std::ostream &stream, const T &value) {
    Print<T>::print(stream, value);
};


template <typename T>
    requires _StdPrintable<T>
struct Print<T> {
    static void print(std::ostream &stream, const T &value) {
        stream << value;
    }
};

template <>
struct Print<uint8_t> {
    static void print(std::ostream &stream, uint8_t value) {
        stream << uint32_t(value);
    }
};
template <>
struct Print<int8_t> {
    static void print(std::ostream &stream, int8_t value) {
        stream << int32_t(value);
    }
};
template <>
struct Print<bool> {
    static void print(std::ostream &stream, bool value) {
        stream << (value ? "true" : "false");
    }
};

} // namespace core
