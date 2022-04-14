#include "message.hpp"

#include <chrono>
#include <iostream>
#include <iomanip>

#include <core/assert.hpp>


using namespace std::chrono_literals;

template <typename OutMsg, typename InMsg>
MessageChannel<OutMsg, InMsg>::MessageChannel(std::unique_ptr<Channel> &&raw_, const size_t max_len) :
    raw_(std::move(raw_)),
    max_len_(max_len) //
{
    send_buffer_.reserve(max_len_);
    recv_buffer_.reserve(max_len_);
}

template <typename OutMsg, typename InMsg>
Result<std::monostate, io::Error> MessageChannel<OutMsg, InMsg>::send(
    const OutMsg &message,
    std::optional<std::chrono::milliseconds> timeout) {

    // TODO: Should we pack multiple messages in one buffer?
    send_buffer_.clear();
    auto res = message.store(send_buffer_);
    if (res.is_err()) {
        return res;
    }
    const size_t length = send_buffer_.size();
    if (length > max_message_length()) {
        return Err(io::Error{io::ErrorKind::UnexpectedEof, "Message size is greater than max length"});
    }
    raw_->timeout = timeout;
    return raw_->stream_write_exact(send_buffer_.data(), length);
}

template <typename OutMsg, typename InMsg>
Result<InMsg, io::Error> MessageChannel<OutMsg, InMsg>::receive(std::optional<std::chrono::milliseconds> timeout) {
    auto begin = std::chrono::steady_clock::now();
    for (;;) {
        // Try to read data from buffer.
        // On success remove data are removed from buffer, on failure data remain in buffer.
        VecDequeView<uint8_t> view = recv_buffer_.view();
        size_t view_len = view.size();
        auto msg_res = InMsg::load(view);
        if (msg_res.is_ok()) {
            recv_buffer_.skip_front(view_len - view.size());
            return msg_res;
        } else if (msg_res.err().kind != io::ErrorKind::UnexpectedEof) {
            return msg_res;
        }

        // Load data from channel
        if (timeout.has_value()) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - begin);
            auto remains = timeout.value() - elapsed;
            if (remains.count() <= 0) {
                break;
            }
            raw_->timeout = remains;
        } else {
            raw_->timeout = std::nullopt;
        }
        auto read_res = recv_buffer_.write_from_stream(*raw_, std::nullopt);
        if (read_res.is_err()) {
            return Err(read_res.unwrap_err());
        }
    }
    return Err(io::Error{io::ErrorKind::TimedOut});
}
