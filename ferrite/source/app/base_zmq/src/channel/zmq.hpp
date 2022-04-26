#pragma once

#include <string>
#include <memory>

#include <zmq.h>

#include <channel/base.hpp>

namespace zmq {

using Context = std::unique_ptr<void, void (*)(void *)>;
Context guard_context(void *context);

using Socket = std::unique_ptr<void, void (*)(void *)>;
Socket guard_socket(void *socket);

} // namespace zmq

class ZmqChannel final : public Channel {
private:
    std::string host_;
    zmq::Context context_;
    zmq::Socket send_socket_;
    zmq::Socket recv_socket_;

    zmq_msg_t last_msg_;
    size_t msg_read_ = 0;

    inline ZmqChannel(
        const std::string &host,
        zmq::Context &&context,
        zmq::Socket &&send_socket,
        zmq::Socket &&recv_socket //
        ) :
        host_(host),
        context_(std::move(context)),
        send_socket_(std::move(send_socket)),
        recv_socket_(std::move(recv_socket)) //
    {}

public:
    virtual ~ZmqChannel() override = default;

    ZmqChannel(ZmqChannel &&) = default;
    ZmqChannel &operator=(ZmqChannel &&) = default;

    static core::Result<ZmqChannel, core::io::Error> create(const std::string &host, uint16_t send_port, uint16_t recv_port);

    virtual core::Result<size_t, core::io::Error> stream_write(std::span<const uint8_t> data) override;
    virtual core::Result<std::monostate, core::io::Error> stream_write_exact(std::span<const uint8_t> data) override;
    virtual core::Result<size_t, core::io::Error> stream_read(std::span<uint8_t> data) override;
};
