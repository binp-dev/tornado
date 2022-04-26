#include "zmq.hpp"

#include <cstring>

#include <core/assert.hpp>
#include <core/log.hpp>

using namespace core;

namespace zmq {

void destroy_context(void *context) {
    core_assert_eq(zmq_ctx_destroy(context), 0);
}
void close_socket(void *socket) {
    core_assert_eq(zmq_close(socket), 0);
}

Context guard_context(void *context) {
    return Context(context, destroy_context);
}
Socket guard_socket(void *socket) {
    return Socket(socket, close_socket);
}

} // namespace zmq

Result<ZmqChannel, io::Error> ZmqChannel::create(const std::string &host, uint16_t send_port, uint16_t recv_port) {
    void *raw_context = zmq_ctx_new();
    if (raw_context == nullptr) {
        return Err(io::Error{io::ErrorKind::Other, "Cannot create ZMQ context"});
    }
    auto context = zmq::guard_context(raw_context);

    auto make_socket = [](zmq::Context &context, const std::string &host, uint32_t port) -> Result<zmq::Socket, io::Error> {
        void *raw_socket = zmq_socket(context.get(), ZMQ_PAIR);
        if (raw_socket == nullptr) {
            return Err(io::Error{io::ErrorKind::Other, "Cannot create ZMQ socket"});
        }
        auto socket = zmq::guard_socket(raw_socket);

        std::string url = core_format("tcp://{}:{}", host, port);
        if (zmq_connect(socket.get(), url.c_str()) != 0) {
            return Err(io::Error{io::ErrorKind::Other, "Error connecting ZMQ socket"});
        }

        return Ok(std::move(socket));
    };

    auto send_socket = make_socket(context, host, send_port);
    if (send_socket.is_err()) {
        return Err(send_socket.unwrap_err());
    }
    auto recv_socket = make_socket(context, host, recv_port);
    if (recv_socket.is_err()) {
        return Err(recv_socket.unwrap_err());
    }

    return Ok(ZmqChannel(host, std::move(context), send_socket.unwrap(), recv_socket.unwrap()));
}


Result<size_t, io::Error> ZmqChannel::stream_write(std::span<const uint8_t> data) {
    auto res = stream_write_exact(data);
    if (res.is_ok()) {
        return Ok(data.size());
    } else {
        return Err(res.unwrap_err());
    }
}

Result<std::monostate, io::Error> ZmqChannel::stream_write_exact(std::span<const uint8_t> data) {
    auto &socket = this->send_socket_;

    zmq_pollitem_t pollitem = {socket.get(), 0, ZMQ_POLLOUT, 0};
    int count = zmq_poll(&pollitem, 1, timeout.has_value() ? timeout.value().count() : -1);
    if (count > 0) {
        if (!(pollitem.revents & ZMQ_POLLOUT)) {
            return Err(io::Error{io::ErrorKind::Other, "Poll bad event"});
        }
    } else if (count == 0) {
        return Err(io::Error{io::ErrorKind::TimedOut});
    } else {
        return Err(io::Error{io::ErrorKind::Other, "Poll error"});
    }

    auto ret = zmq_send(socket.get(), data.data(), data.size(), !timeout ? 0 : ZMQ_NOBLOCK);
    if (ret <= 0) {
        return Err(io::Error{io::ErrorKind::Other, core_format("Error send: {}", ret)});
    }
    return Ok(std::monostate{});
}
Result<size_t, io::Error> ZmqChannel::stream_read(std::span<uint8_t> data) {
    if (data.size() == 0) {
        return Ok<size_t>(0);
    }
    if (this->msg_read_ == 0) {
        auto &socket = this->recv_socket_;

        zmq_pollitem_t pollitem = {socket.get(), 0, ZMQ_POLLIN, 0};
        int count = zmq_poll(&pollitem, 1, timeout.has_value() ? timeout.value().count() : -1);
        if (count > 0) {
            if (!(pollitem.revents & ZMQ_POLLIN)) {
                return Err(io::Error{io::ErrorKind::Other, "Poll bad event"});
            }
        } else if (count == 0) {
            return Err(io::Error{io::ErrorKind::TimedOut});
        } else {
            return Err(io::Error{io::ErrorKind::Other, "Poll error"});
        }

        core_assert_eq(zmq_msg_init(&this->last_msg_), 0);
        int ret = zmq_msg_recv(&this->last_msg_, socket.get(), ZMQ_NOBLOCK);
        if (ret <= 0) {
            return Err(io::Error{io::ErrorKind::Other, "Error receive"});
        }
    }

    size_t msg_len = zmq_msg_size(&this->last_msg_);
    const uint8_t *msg_data = static_cast<const uint8_t *>(zmq_msg_data(&this->last_msg_));
    size_t read_len = std::min(msg_len - this->msg_read_, data.size());
    memcpy(data.data(), msg_data + this->msg_read_, read_len);
    this->msg_read_ += read_len;
    if (this->msg_read_ >= msg_len) {
        zmq_msg_close(&this->last_msg_);
        this->msg_read_ = 0;
    }
    return Ok(read_len);
}
