#pragma once

#include <cstdlib>
#include <cstdint>

#include <utils/exception.hpp>


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
        IoError() : Exception("Channel::IoError") {}
        IoError(const char *msg) : Exception("Channel::IoError" + std::string(msg)) {}
        IoError(std::string msg) : Exception("Channel::IoError" + msg) {}
    };
    class TimedOut : public Exception {
        public:
        template <typename ... Args>
        TimedOut() : Exception("Channel::TimedOut") {}
        TimedOut(const char *msg) : Exception("Channel::TimedOut" + std::string(msg)) {}
        TimedOut(std::string msg) : Exception("Channel::TimedOut" + msg) {}
    };

    Channel() = default;
    Channel(const Channel &ch) = delete;
    Channel &operator=(const Channel &ch) = delete;
    virtual ~Channel() = default;

    virtual void send(const uint8_t *bytes, size_t length, int timeout) = 0;
    virtual size_t receive(uint8_t *bytes, size_t max_length, int timeout) = 0;
};
