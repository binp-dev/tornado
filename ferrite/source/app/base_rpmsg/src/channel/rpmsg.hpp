#pragma once

#include <string>

#include <termios.h>

#include <channel/base.hpp>


class RpmsgChannel final : public Channel {
private:
    int fd_;
    struct termios tty_;

    inline RpmsgChannel(int fd, struct termios tty) : fd_(fd), tty_(tty) {}
    void close();

public:
    virtual ~RpmsgChannel() override;

    RpmsgChannel(RpmsgChannel &&other);
    RpmsgChannel &operator=(RpmsgChannel &&other);

    static Result<RpmsgChannel, Error> create(const std::string &dev);

    virtual Result<std::monostate, Error> send(const uint8_t *bytes, size_t length, std::optional<std::chrono::milliseconds> timeout) override;
    virtual Result<size_t, Error> receive(uint8_t *bytes, size_t max_length, std::optional<std::chrono::milliseconds> timeout) override;
};
