#pragma once

#include <vector>
#include <memory>

#include <core/collections/vec.hpp>
#include <core/collections/vec_deque.hpp>

#include "base.hpp"


template <typename OutMsg, typename InMsg>
class MessageChannel final {
private:
    std::unique_ptr<Channel> raw_;

    VecStream send_buffer_;
    VecDequeStream recv_buffer_;

    size_t max_len_;

public:
    explicit MessageChannel(std::unique_ptr<Channel> &&raw_, size_t max_len);

    size_t max_message_length() const {
        return max_len_;
    }

    Channel &raw_channel() {
        return *raw_;
    }
    const Channel &raw_channel() const {
        return *raw_;
    }

    Result<std::monostate, io::Error> send(const OutMsg &message, std::optional<std::chrono::milliseconds> timeout);
    Result<InMsg, io::Error> receive(std::optional<std::chrono::milliseconds> timeout);
};

#include "message.hxx"
