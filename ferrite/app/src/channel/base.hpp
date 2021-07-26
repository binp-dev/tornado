#pragma once

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <chrono>
#include <iostream>

#include <core/io.hpp>
#include <core/result.hpp>

#include <ipp.hpp>

class Channel {
public:
    enum class ErrorKind {
        IoError,
        OutOfBounds,
        ParseError,
        TimedOut
    };
    struct Error final {
        ErrorKind kind;
        std::string message;
    };

private:
    std::vector<uint8_t> buffer_;
    size_t data_start_ = 0;
    size_t data_end_ = 0;

public:
    explicit Channel(size_t max_length);
    virtual ~Channel() = default;
    
    Channel(Channel &&ch) = default;
    Channel &operator=(Channel &&ch) = default;

    Channel(const Channel &ch) = delete;
    Channel &operator=(const Channel &ch) = delete;

    inline size_t max_length() const { return buffer_.size(); }

    virtual Result<std::monostate, Error> send_raw(const uint8_t *bytes, size_t length, std::optional<std::chrono::milliseconds> timeout) = 0;
    virtual Result<size_t, Error> receive_raw(uint8_t *bytes, size_t max_length, std::optional<std::chrono::milliseconds> timeout) = 0;

    Result<std::monostate, Error> send(const ipp::MsgAppAny &message, std::optional<std::chrono::milliseconds> timeout);
    Result<ipp::MsgMcuAny, Error> receive(std::optional<std::chrono::milliseconds> timeout);
};

template <>
struct IsWritable<Channel::ErrorKind> : std::true_type {};
std::ostream &operator<<(std::ostream &o, const Channel::ErrorKind &ek);

template <>
struct IsWritable<Channel::Error> : std::true_type {};
std::ostream &operator<<(std::ostream &o, const Channel::Error &e);
