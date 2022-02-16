#include "message.hpp"

#include <chrono>

#include <core/assert.hpp>


using namespace std::chrono_literals;

template <typename OutMsg, typename InMsg>
MessageChannel<OutMsg, InMsg>::MessageChannel(std::unique_ptr<Channel> &&raw, const size_t max_length) :
    raw(std::move(raw)),
    send_buffer_(max_length, 0),
    recv_buffer_(8 * max_length, 0) {}

template <typename OutMsg, typename InMsg>
size_t MessageChannel<OutMsg, InMsg>::max_length() const {
    return send_buffer_.size();
}

template <typename OutMsg, typename InMsg>
Result<std::monostate, Channel::Error> MessageChannel<OutMsg, InMsg>::send(
    const OutMsg &message,
    std::optional<std::chrono::milliseconds> timeout) {

    // TODO: Should we pack multiple messages in one buffer?
    const size_t length = message.packed_size();
    if (length > send_buffer_.size()) {
        return Err(Channel::Error{Channel::ErrorKind::OutOfBounds, "Message size is greater than buffer length"});
    }
    message.store((typename OutMsg::Raw *)send_buffer_.data());
    return raw->send(send_buffer_.data(), length, timeout);
}

template <typename OutMsg, typename InMsg>
Result<std::monostate, Channel::Error> MessageChannel<OutMsg, InMsg>::fill_recv_buffer(
    const std::optional<std::chrono::milliseconds> timeout) {

    bool trailing = false;
    while (data_end_ < recv_buffer_.size()) {
        auto recv_res = raw->receive(
            recv_buffer_.data() + data_end_,
            recv_buffer_.size() - data_end_,
            trailing ? 0ms : timeout);
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

template <typename OutMsg, typename InMsg>
Result<InMsg, Channel::Error> MessageChannel<OutMsg, InMsg>::receive(std::optional<std::chrono::milliseconds> timeout) {

    if (data_start_ >= data_end_) {
        // Incoming message should fit recv_buffer_
        data_start_ = 0;
        data_end_ = 0;
        try_unwrap(fill_recv_buffer(timeout));
    }
    const auto *raw_msg = (typename InMsg::Raw *)(recv_buffer_.data() + data_start_);
    const size_t msg_len = ipp_mcu_msg_size(raw_msg);
    assert_true(data_start_ + msg_len <= data_end_);
    auto msg = InMsg::load(raw_msg);
    data_start_ += msg_len;
    return Ok(std::move(msg));
}
