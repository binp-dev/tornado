#pragma once

#include <string>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>

#include "channel.hpp"


class RpmsgChannel : public Channel {
private:
    int fd;
    struct termios tty;

public:
    RpmsgChannel(const std::string &dev) {
        fd = open(dev.c_str(), O_NOCTTY | O_RDWR);// | O_NONBLOCK);
        if (fd < 0) {
            throw Channel::IoError("Open error");
        }
        cfmakeraw(&tty);
        if (tcsetattr(fd, TCSAFLUSH, &tty) < 0) {
            throw Channel::IoError("Error tcsetattr");
        }

    }
    ~RpmsgChannel() override {
        close(fd);
    }

    void send(const uint8_t *bytes, size_t length, int timeout) override {
        pollfd pfd = { .fd = fd, .events = POLLOUT };
        int pr = poll(&pfd, 1, timeout);
        if (pr > 0) {
            if (!(pfd.revents & POLLOUT)) {
                throw Channel::IoError("Poll bad event");
            }
        } else if (pr == 0) {
            throw Channel::TimedOut();
        } else {
            throw Channel::IoError("Poll error");
        }

        int wr = write(fd, bytes, length);
        if (wr <= 0) {
            throw Channel::IoError("Write error");
        } else if (wr != (int)length) {
            throw Channel::IoError("Cannot write full message");
        }
    }
    size_t receive(uint8_t *bytes, size_t max_length, int timeout) override {
        pollfd pfd = { .fd = fd, .events = POLLIN };
        int pr = poll(&pfd, 1, timeout);
        if (pr > 0) {
            if (!(pfd.revents & POLLIN)) {
                throw Channel::IoError("Poll bad event");
            }
        } else if (pr == 0) {
            throw Channel::TimedOut();
        } else {
            throw Channel::IoError("Poll error");
        }

        int rr = read(fd, bytes, max_length);
        if (rr <= 0) {
            throw Channel::IoError("Read error");
        }

        return (size_t)rr;
    }
};
