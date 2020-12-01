#pragma once

#include <string>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>

#include "base.hpp"


class RpmsgChannel final : public Channel {
private:
    int fd_;
    struct termios tty_;

    RpmsgChannel(int fd, struct termios tty) : fd_(fd), tty_(tty) {}

public:
    Result<RpmsgChannel, Error> create(const std::string &dev) {
        //std::cout << "Init RPMSG channel: " << dev << std::endl;
        int fd = open(dev.c_str(), O_NOCTTY | O_RDWR);// | O_NONBLOCK);
        if (fd < 0) {
            return Err(Error{ErrorKind::IoError, "Open error"});
        }
        struct termios tty;
        cfmakeraw(&tty);
        if (tcsetattr(fd, TCSAFLUSH, &tty) < 0) {
            return Err(Error{ErrorKind::IoError, "Error tcsetattr"});
        }
        return Ok(RpmsgChannel(fd, tty));
    }
    virtual ~RpmsgChannel() override {
        close(this->fd_);
    }

    virtual Result<std::monostate, Error> send(const uint8_t *bytes, size_t length, std::optional<std::chrono::milliseconds> timeout) override {
        //std::cout << "Send data" << std::endl;

        pollfd pfd = { .fd = this->fd_, .events = POLLOUT };
        int pr = poll(&pfd, 1, timeout ? int(timeout->count()) : -1);
        if (pr > 0) {
            if (!(pfd.revents & POLLOUT)) {
                return Err(Error{ErrorKind::IoError, "Poll bad event"});
            }
        } else if (pr == 0) {
            return Err(Error{ErrorKind::TimedOut, });
        } else {
            return Err(Error{ErrorKind::IoError, "Poll error"});
        }

        int wr = write(this->fd_, bytes, length);
        if (wr <= 0) {
            return Err(Error{ErrorKind::IoError, "Write error"});
        } else if (wr != (int)length) {
            return Err(Error{ErrorKind::IoError, "Cannot write full message"});
        }
    }
    virtual Result<size_t, Error> receive(uint8_t *bytes, size_t max_length, std::optional<std::chrono::milliseconds> timeout) override {
        pollfd pfd = { .fd = this->fd_, .events = POLLIN };
        int pr = poll(&pfd, 1, timeout ? int(timeout->count()) : -1);
        if (pr > 0) {
            if (!(pfd.revents & POLLIN)) {
                return Err(Error{ErrorKind::IoError, "Poll bad event"});
            }
        } else if (pr == 0) {
            return Err(Error{ErrorKind::TimedOut, });
        } else {
            return Err(Error{ErrorKind::IoError, "Poll error"});
        }

        int rr = read(this->fd_, bytes, max_length);
        if (rr <= 0) {
            return Err(Error{ErrorKind::IoError, "Read error"});
        }

        //std::cout << "Received data" << std::endl;

        return Ok(size_t(rr));
    }
};
