#pragma once

#include <cstdlib>
#include <cstdint>
#include <chrono>
#include <exception>

typedef std::chrono::milliseconds msec;

class Channel {
public:
    class Exception : public std::exception {};
    class IoError : public Exception {};
    class TimedOut : public Exception {};

    Channel() = default;
    Channel(const Channel &ch) = delete;
    Channel &operator=(const Channel &ch) = delete;
    virtual ~Channel() = default;

    virtual void send(const uint8_t *bytes, size_t length, msec timeout) = 0;
    virtual size_t receive(uint8_t *bytes, size_t max_length, msec timeout) = 0;
};
