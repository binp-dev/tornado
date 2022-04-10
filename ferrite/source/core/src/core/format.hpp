#pragma once

// TODO: Migrate to std::format when supported.

#include <string>
#include <iostream>
#include <type_traits>
#include <concepts>
#include <cstdint>
#include <optional>

// Concepts

namespace format_impl {

template <typename T>
concept StdPrintable = requires(std::ostream &stream, const T &value) {
    { stream << value } -> std::same_as<std::ostream &>;
};

} // namespace format_impl

template <typename T>
struct Print {};

template <typename T>
concept Printable = requires(std::ostream &stream, T value) {
    Print<T>::print(stream, value);
};


// Print basic impls

template <typename T>
    requires format_impl::StdPrintable<T>
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
