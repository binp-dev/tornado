#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <optional>

#include "result.hpp"
#include "stream.hpp"
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
    //! @return Number of bytes read or error.
    virtual Result<size_t, Error> read(uint8_t *data, size_t len) = 0;
};

class Write {
public:
    //! Try to write at most `len` bytes from `data` buffer.
    //! @return Number of bytes written or error.
    virtual Result<size_t, Error> write(const uint8_t *data, size_t len) = 0;
};

class ReadExact : public virtual Read {
public:
    //! Try to read exactly `len` bytes into `data` buffer or return error.
    virtual Result<std::monostate, Error> read_exact(uint8_t *data, size_t len) = 0;
};

class WriteExact : public virtual Write {
public:
    //! Try to write exactly `len` bytes from `data` buffer or return error.
    virtual Result<std::monostate, Error> write_exact(const uint8_t *data, size_t len) = 0;
};

class ReadFrom {
public:
    //! Read bytes from given stream into `this`.
    //! @param len Number of bytes to read. If `nullopt` then read as much as possible.
    virtual Result<size_t, Error> read_from(Read &stream, std::optional<size_t> len) = 0;
};

class WriteInto {
public:
    //! Write bytes from `this` into given stream.
    //! @param len Number of bytes to write. If `nullopt` then write as much as possible.
    virtual Result<size_t, Error> write_into(Write &stream, std::optional<size_t> len) = 0;
};

struct StreamReadWrapper final : StreamRead<uint8_t> {
    Read &read;
    std::optional<Error> error = std::nullopt;

    StreamReadWrapper(Read &r) : read(r) {}
    [[nodiscard]] size_t stream_read(uint8_t *data, size_t len) override;
};

struct StreamWriteWrapper final : StreamWrite<uint8_t> {
    Write &write;
    std::optional<Error> error = std::nullopt;

    StreamWriteWrapper(Write &w) : write(w) {}
    [[nodiscard]] size_t stream_write(const uint8_t *data, size_t len) override;
};

} // namespace io

std::ostream &operator<<(std::ostream &o, const io::ErrorKind &ek);

std::ostream &operator<<(std::ostream &o, const io::Error &e);
