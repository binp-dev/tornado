#include "base.hpp"

#include <iostream>

#include <core/assert.hpp>

using namespace std::chrono_literals;

Channel::Channel(const size_t max_length) :
    send_buffer_(max_length, 0),
    recv_buffer_(8 * max_length, 0)
{}

// TODO: Should we pack multiple messages in one buffer?
Result<std::monostate, Channel::Error> Channel::send(const ipp::AppMsg &message, std::optional<std::chrono::milliseconds> timeout) {
    const size_t length = message.packed_size();
    if (length > send_buffer_.size()) {
        return Err(Error{ErrorKind::OutOfBounds, "Message size is greater than buffer length"});
    }
    message.store((IppAppMsg *)send_buffer_.data());
    return this->send_raw(send_buffer_.data(), length, timeout);
}

Result<std::monostate, Channel::Error> Channel::fill_recv_buffer(const std::optional<std::chrono::milliseconds> timeout) {
    bool trailing = false;
    while (data_end_ < recv_buffer_.size()) {
        auto recv_res = this->receive_raw(recv_buffer_.data() + data_end_, recv_buffer_.size() - data_end_, trailing ? 0ms : timeout);
        if (recv_res.is_err()) {
            if (trailing && recv_res.err().kind == Channel::ErrorKind::TimedOut) {
                break;
            } else {
                return Err(std::move(recv_res.err()));
            }
        }
        data_end_ += recv_res.ok();
        assert_true(data_end_ <= recv_buffer_.size());
        trailing = true;
    }
    return Ok(std::monostate{});
}

Result<ipp::McuMsg, Channel::Error> Channel::receive(std::optional<std::chrono::milliseconds> timeout) {
    if (data_start_ >= data_end_) {
        // Incoming message should fit recv_buffer_
        data_start_ = 0;
        data_end_ = 0;
        try_unwrap(fill_recv_buffer(timeout));
    }
    const auto *raw_msg = (IppMcuMsg *)(recv_buffer_.data() + data_start_);
    const size_t msg_len = ipp_mcu_msg_size(raw_msg);
    assert_true(data_start_ + msg_len <= data_end_);
    auto msg = ipp::McuMsg::load(raw_msg);
    data_start_ += msg_len;
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
