#include "base.hpp"

#include <iostream>

#include <core/assert.hpp>

Channel::Channel(const size_t max_length) : buffer_(max_length, 0) {}

// TODO: Should we pack multiple messages in one buffer?
Result<std::monostate, Channel::Error> Channel::send(const ipp::MsgAppAny &message, std::optional<std::chrono::milliseconds> timeout) {
    const size_t length = message.length();
    if (length > buffer_.size()) {
        return Err(Error{ErrorKind::OutOfBounds, "Message size is greater than buffer length"});
    }
    message.store(buffer_.data());
    return this->send_raw(buffer_.data(), length, timeout);
}

Result<ipp::MsgMcuAny, Channel::Error> Channel::receive(std::optional<std::chrono::milliseconds> timeout) {
    if (data_start_ >= data_end_) {
        const auto recv_res = this->receive_raw(buffer_.data(), buffer_.size(), timeout);
        if (recv_res.is_err()) {
            return Err(recv_res.err());
        }
        data_start_ = 0;
        data_end_ = recv_res.ok();
        // TODO: We don't support multi-buffer messages yet.
    }
    auto msg_res = ipp::MsgMcuAny::load(buffer_.data() + data_start_, data_end_ - data_start_);
    // TODO: Skip all buffers on error.
    if (msg_res.index() != 0) {
        switch (std::get<1>(msg_res)) {
        case IPP_LOAD_OK: unreachable();
        case IPP_LOAD_OUT_OF_BOUNDS: return Err(Error{ErrorKind::OutOfBounds, "Message size is greater than received data length"});
        case IPP_LOAD_PARSE_ERROR: return Err(Error{ErrorKind::ParseError, "Received message parse error"});
        }
    }
    auto &msg = std::get<0>(msg_res);
    data_start_ += msg.length();
    assert_true(data_start_ <= data_end_);
    return Ok(std::move(msg));
}

std::ostream &operator<<(std::ostream &o, const Channel::ErrorKind &ek) {
    switch (ek)
    {
    case Channel::ErrorKind::IoError:
        o << "IoError";
        break;
    case Channel::ErrorKind::OutOfBounds:
        o << "OutOfBounds";
        break;
    case Channel::ErrorKind::ParseError:
        o << "ParseError";
        break;
    case Channel::ErrorKind::TimedOut:
        o << "TimedOut";
        break;
    }
    return o;
}

std::ostream &operator<<(std::ostream &o, const Channel::Error &e) {
    return o << "Channel::Error(" << e.kind << ": " << e.message << ")";
}
