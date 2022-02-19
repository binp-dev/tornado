#include "message.hpp"

#include <chrono>

#include <core/assert.hpp>


using namespace std::chrono_literals;

template <typename OutMsg, typename InMsg>
MessageChannel<OutMsg, InMsg>::MessageChannel(std::unique_ptr<Channel> &&raw, const size_t max_len) :
    raw(std::move(raw)),
    max_len_(max_len) //
{
    send_buffer_.vector.reserve(max_len_);
    recv_buffer_.queue.reserve(max_len_);
}

template <typename OutMsg, typename InMsg>
size_t MessageChannel<OutMsg, InMsg>::max_message_length() const {
    return max_len_;
}

template <typename OutMsg, typename InMsg>
Result<std::monostate, io::Error> MessageChannel<OutMsg, InMsg>::send(
    const OutMsg &message,
    std::optional<std::chrono::milliseconds> timeout) {

    // TODO: Should we pack multiple messages in one buffer?
    send_buffer_.vector.clear();
    auto res = message.store(send_buffer_);
    if (res.is_err()) {
        return res;
    }
    const size_t length = send_buffer_.vector.size();
    if (length > max_message_length()) {
        return Err(io::Error{io::ErrorKind::UnexpectedEof, "Message size is greater than max length"});
    }
    raw->timeout = timeout;
    return raw->write_exact(send_buffer_.vector.data(), length);
}

template <typename OutMsg, typename InMsg>
Result<InMsg, io::Error> MessageChannel<OutMsg, InMsg>::receive(std::optional<std::chrono::milliseconds> timeout) {
    auto begin = std::chrono::steady_clock::now();
    for (;;) {
        // Try to read data from buffer
        auto msg_res = InMsg::load(recv_buffer_);
        if (msg_res.is_ok() || msg_res.err().kind != io::ErrorKind::UnexpectedEof) {
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
            raw->timeout = remains;
        } else {
            raw->timeout = std::nullopt;
        }
        auto read_res = recv_buffer_.read_from(*raw, std::nullopt);
        if (read_res.is_err()) {
            return Err(read_res.unwrap_err());
        }
    }
    return Err(io::Error{io::ErrorKind::TimedOut});
}
