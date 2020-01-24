#pragma once

#include <cstdlib>
#include <cstdint>
#include <chrono>

#include <utils/exception.hpp>

typedef std::chrono::milliseconds msec;

class Channel {
public:
    class Exception : public ::Exception {
        public:
        template <typename ... Args>
        Exception(Args ... args) : ::Exception(args ...) {}
    };
    class IoError : public Exception {
        public:
        template <typename ... Args>
        IoError(Args ... args) : Exception(args ...) {}
    };
    class TimedOut : public Exception {
        public:
        template <typename ... Args>
        TimedOut(Args ... args) : Exception(args ...) {}
    };

    Channel() = default;
    Channel(const Channel &ch) = delete;
    Channel &operator=(const Channel &ch) = delete;
    virtual ~Channel() = default;

    virtual void send(const uint8_t *bytes, size_t length, msec timeout) = 0;
    virtual size_t receive(uint8_t *bytes, size_t max_length, msec timeout) = 0;
};
