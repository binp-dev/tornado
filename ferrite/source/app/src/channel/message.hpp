#pragma once

#include <vector>
#include <memory>

#include "base.hpp"


template <typename OutMsg, typename InMsg>
class MessageChannel final {
private:
    std::unique_ptr<Channel> raw;

    std::vector<uint8_t> send_buffer_;

    std::vector<uint8_t> recv_buffer_;
    size_t data_start_ = 0;
    size_t data_end_ = 0;

public:
    explicit MessageChannel(std::unique_ptr<Channel> &&raw, size_t max_length);

    size_t max_length() const;

    Result<std::monostate, Channel::Error> send(const OutMsg &message, std::optional<std::chrono::milliseconds> timeout);
    Result<InMsg, Channel::Error> receive(std::optional<std::chrono::milliseconds> timeout);

private:
    Result<std::monostate, Channel::Error> fill_recv_buffer(std::optional<std::chrono::milliseconds> timeout);
};

#include "message.hxx"
