#include "base.hpp"

#include <iostream>

#include <core/assert.hpp>

Channel::Channel(const size_t max_length) : buffer_(max_length, 0) {}

// TODO: Should we pack multiple messages in one buffer?
Result<std::monostate, Channel::Error> Channel::send(const ipp::AppMsg &message, std::optional<std::chrono::milliseconds> timeout) {
    const size_t length = message.packed_size();
    if (length > buffer_.size()) {
        return Err(Error{ErrorKind::OutOfBounds, "Message size is greater than buffer length"});
    }
    message.store((IppAppMsg *)buffer_.data());
    return this->send_raw(buffer_.data(), length, timeout);
}

Result<ipp::McuMsg, Channel::Error> Channel::receive(std::optional<std::chrono::milliseconds> timeout) {
    if (data_start_ >= data_end_) {
        const auto recv_res = this->receive_raw(buffer_.data(), buffer_.size(), timeout);
        if (recv_res.is_err()) {
            return Err(recv_res.err());
        }
        data_start_ = 0;
        data_end_ = recv_res.ok();
        // FIXME: We don't support multi-buffer messages yet.
    }
    const auto *raw_msg = (IppMcuMsg *)(buffer_.data() + data_start_);
    const size_t msg_len = ipp_mcu_msg_size(raw_msg);
    assert_true(msg_len <= data_end_ - data_start_);
    auto msg = ipp::McuMsg::load(raw_msg);
    data_start_ += msg_len;
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
