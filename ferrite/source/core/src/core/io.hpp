#pragma once

#include <cstdlib>
#include <iostream>
#include <type_traits>

class ReadStream {
public:
    //! Try to read at most `len` bytes into `data` buffer.
    //! @return Number of bytes read.
    virtual size_t read(uint8_t *data, size_t len) = 0;
};

class WriteStream {
public:
    //! Try to write at most `len` bytes from `data` buffer.
    //! @return Number of bytes written.
    virtual size_t write(const uint8_t *data, size_t len) = 0;
};

//! TODO: Rename to `Display`?
template <typename T>
struct IsWritable : std::false_type {};

template <>
struct IsWritable<std::string> : std::true_type {};

template <typename T>
constexpr bool is_writable = IsWritable<T>::value;
