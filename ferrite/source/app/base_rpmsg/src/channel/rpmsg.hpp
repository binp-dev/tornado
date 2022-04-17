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

    static core::Result<RpmsgChannel, core::io::Error> create(const std::string &dev);

    virtual core::Result<size_t, core::io::Error> stream_write(std::span<const uint8_t> data) override;
    virtual core::Result<std::monostate, core::io::Error> stream_write_exact(std::span<const uint8_t> data) override;
    virtual core::Result<size_t, core::io::Error> stream_read(std::span<uint8_t> data) override;
};
