#pragma once

#include <cstdint>

#include <core/result.hpp>

enum class IoError {};

class Read {
public:
    //! Try to read at most `len` bytes into `data` buffer.
    //! @return Number of bytes read or error.
    virtual Result<size_t, IoError> read(uint8_t *data, size_t len) = 0;
};

class Write {
public:
    //! Try to write at most `len` bytes from `data` buffer.
    //! @return Number of bytes written or error.
    virtual Result<size_t, IoError> write(const uint8_t *data, size_t len) = 0;
};
