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

    static Result<RpmsgChannel, io::Error> create(const std::string &dev);

    virtual Result<size_t, io::Error> stream_write(const uint8_t *data, size_t len) override;
    virtual Result<std::monostate, io::Error> stream_write_exact(const uint8_t *data, size_t len) override;
    virtual Result<size_t, io::Error> stream_read(uint8_t *data, size_t len) override;
};
