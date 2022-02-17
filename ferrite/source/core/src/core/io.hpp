#pragma once

#include <cstdint>
#include <string>
#include <iostream>

#include "result.hpp"
#include "fmt.hpp"

namespace io {

enum class ErrorKind {
    NotFound,
    InvalidData,
    UnexpectedEof,
    TimedOut,
    Other,
};

struct Error final {
    ErrorKind kind;
    std::string message;

    inline Error(ErrorKind kind) : kind(kind) {}
    inline Error(ErrorKind kind, std::string message) : kind(kind), message(message) {}
};

class Read {
public:
    //! Try to read at most `len` bytes into `data` buffer.
    //! @param exact Read exactly `len` of bytes or return error.
    //! @return Number of bytes read or error.
    virtual Result<size_t, Error> read(uint8_t *data, size_t len) = 0;
};

class Write {
public:
    //! Try to write at most `len` bytes from `data` buffer.
    //! @param exact Read exactly `len` of bytes or return error.
    //! @return Number of bytes written or error.
    virtual Result<size_t, Error> write(const uint8_t *data, size_t len) = 0;
};

class ReadExact : public Read {
public:
    //! Try to read exactly `len` bytes into `data` buffer.
    virtual Result<std::monostate, Error> read_exact(uint8_t *data, size_t len) = 0;

    Result<size_t, Error> read(uint8_t *data, size_t len) override;
};

class WriteExact : public Write {
public:
    //! Try to write exactly `len` bytes from `data` buffer.
    virtual Result<std::monostate, Error> write_exact(const uint8_t *data, size_t len) = 0;

    Result<size_t, Error> write(const uint8_t *data, size_t len) override;
};

} // namespace io

template <>
struct Display<io::ErrorKind> : std::true_type {};
std::ostream &operator<<(std::ostream &o, const io::ErrorKind &ek);

template <>
struct Display<io::Error> : std::true_type {};
std::ostream &operator<<(std::ostream &o, const io::Error &e);
