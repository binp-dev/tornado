#pragma once

#include <string>
#include <memory>

#include <zmq.h>

#include <channel/base.hpp>

namespace zmq_helper {

struct ContextDestroyer final {
    void operator()(void *context) const;
};
using ContextGuard = std::unique_ptr<void, ContextDestroyer>;
ContextGuard guard_context(void *context);

struct SocketCloser final {
    void operator()(void *socket) const;
};
using SocketGuard = std::unique_ptr<void, SocketCloser>;
SocketGuard guard_socket(void *socket);

} // namespace zmq_helper

class ZmqChannel final : public Channel {
private:
    std::string host_;
    zmq_helper::ContextGuard context_;
    zmq_helper::SocketGuard socket_;

    zmq_msg_t last_msg_;
    size_t msg_read_ = 0;

    inline ZmqChannel(const std::string &host, zmq_helper::ContextGuard &&context, zmq_helper::SocketGuard &&socket) :
        host_(host),
        context_(std::move(context)),
        socket_(std::move(socket)) {}

public:
    virtual ~ZmqChannel() override = default;

    ZmqChannel(ZmqChannel &&) = default;
    ZmqChannel &operator=(ZmqChannel &&) = default;

    static core::Result<ZmqChannel, core::io::Error> create(const std::string &host);

    virtual core::Result<size_t, core::io::Error> stream_write(const uint8_t *data, size_t len) override;
    virtual core::Result<std::monostate, core::io::Error> stream_write_exact(const uint8_t *data, size_t len) override;
    virtual core::Result<size_t, core::io::Error> stream_read(uint8_t *data, size_t len) override;
};
