#pragma once

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <chrono>
#include <iostream>

#include <core/fmt.hpp>
#include <core/result.hpp>

class Channel {
public:
    enum class ErrorKind {
        IoError,
        OutOfBounds,
        ParseError,
        TimedOut,
    };
    struct Error final {
        ErrorKind kind;
        std::string message;
    };

public:
    Channel() = default;
    virtual ~Channel() = default;

    Channel(Channel &&ch) = default;
    Channel &operator=(Channel &&ch) = default;

    Channel(const Channel &ch) = delete;
    Channel &operator=(const Channel &ch) = delete;

    virtual Result<std::monostate, Error> send(
        const uint8_t *bytes,
        size_t length,
        std::optional<std::chrono::milliseconds> timeout) = 0;
    virtual Result<size_t, Error> receive(
        uint8_t *bytes,
        size_t max_length,
        std::optional<std::chrono::milliseconds> timeout) = 0;
};

template <>
struct Display<Channel::ErrorKind> : std::true_type {};
std::ostream &operator<<(std::ostream &o, const Channel::ErrorKind &ek);

template <>
struct Display<Channel::Error> : std::true_type {};
std::ostream &operator<<(std::ostream &o, const Channel::Error &e);
