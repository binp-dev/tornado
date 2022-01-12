#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#include "rpmsg.hpp"


Result<RpmsgChannel, RpmsgChannel::Error> RpmsgChannel::create(const std::string &dev, const size_t max_length) {
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
    return Ok(RpmsgChannel(fd, tty, max_length));
}
void RpmsgChannel::close() {
    if (this->fd_ >= 0) {
        ::close(this->fd_);
        this->fd_ = -1;
    }
}
RpmsgChannel::~RpmsgChannel() {
    this->close();
}

RpmsgChannel::RpmsgChannel(RpmsgChannel &&other) :
    Channel(other.max_length()),
    fd_(other.fd_),
    tty_(other.tty_)
{
    other.fd_ = -1;
}
RpmsgChannel &RpmsgChannel::operator=(RpmsgChannel &&other) {
    this->close();
    Channel::operator=(std::move(other));
    this->fd_ = other.fd_;
    this->tty_ = other.tty_;
    other.fd_ = -1;
    return *this;
}

Result<std::monostate, RpmsgChannel::Error> RpmsgChannel::send_raw(
    const uint8_t *bytes, size_t length, std::optional<std::chrono::milliseconds> timeout
) {
    //std::cout << "Send data" << std::endl;

    pollfd pfd = {this->fd_, POLLOUT, 0};
    int pr = poll(&pfd, 1, timeout ? int(timeout->count()) : -1);
    if (pr > 0) {
        if (!(pfd.revents & POLLOUT)) {
            return Err(Error{ErrorKind::IoError, "Poll bad event"});
        }
    } else if (pr == 0) {
        return Err(Error{ErrorKind::TimedOut, ""});
    } else {
        return Err(Error{ErrorKind::IoError, "Poll error"});
    }

    int wr = write(this->fd_, bytes, length);
    if (wr <= 0) {
        return Err(Error{ErrorKind::IoError, "Write error"});
    } else if (wr != (int)length) {
        return Err(Error{ErrorKind::IoError, "Cannot write full message"});
    }

    return Ok(std::monostate{});
}
Result<size_t, RpmsgChannel::Error> RpmsgChannel::receive_raw(
    uint8_t *bytes, size_t max_length, std::optional<std::chrono::milliseconds> timeout
) {
    pollfd pfd = {this->fd_, POLLIN, 0};
    int pr = poll(&pfd, 1, timeout.has_value() ? int(timeout->count()) : -1);
    if (pr > 0) {
        if (!(pfd.revents & POLLIN)) {
            return Err(Error{ErrorKind::IoError, "Poll bad event"});
        }
    } else if (pr == 0) {
        return Err(Error{ErrorKind::TimedOut, ""});
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
