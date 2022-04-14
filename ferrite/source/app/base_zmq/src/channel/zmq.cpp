#include "zmq.hpp"

#include <cstring>

#include <core/assert.hpp>


namespace zmq_helper {

void ContextDestroyer::operator()(void *context) const {
    core_assert_eq(zmq_ctx_destroy(context), 0);
}
void SocketCloser::operator()(void *socket) const {
    core_assert_eq(zmq_close(socket), 0);
}

static const ContextDestroyer CONTEXT_DESTROYER{};
static const SocketCloser SOCKET_CLOSER{};

ContextGuard guard_context(void *context) {
    return ContextGuard(context, CONTEXT_DESTROYER);
}
SocketGuard guard_socket(void *socket) {
    return SocketGuard(socket, SOCKET_CLOSER);
}

} // namespace zmq_helper

Result<ZmqChannel, io::Error> ZmqChannel::create(const std::string &host) {
    void *raw_context = zmq_ctx_new();
    if (raw_context == nullptr) {
        return Err(io::Error{io::ErrorKind::Other, "Cannot create ZMQ context"});
    }
    auto context = zmq_helper::guard_context(raw_context);

    void *raw_socket = zmq_socket(context.get(), ZMQ_PAIR);
    if (raw_socket == nullptr) {
        return Err(io::Error{io::ErrorKind::Other, "Cannot create ZMQ socket"});
    }
    auto socket = zmq_helper::guard_socket(raw_socket);

    if (zmq_connect(socket.get(), host.c_str()) != 0) {
        return Err(io::Error{io::ErrorKind::Other, "Error connecting ZMQ socket"});
    }

    return Ok(ZmqChannel(host, std::move(context), std::move(socket)));
}


Result<size_t, io::Error> ZmqChannel::stream_write(const uint8_t *data, size_t len) {
    auto res = stream_write_exact(data, len);
    if (res.is_ok()) {
        return Ok(len);
    } else {
        return Err(res.unwrap_err());
    }
}

Result<std::monostate, io::Error> ZmqChannel::stream_write_exact(const uint8_t *data, size_t len) {
    zmq_pollitem_t pollitem = {this->socket_.get(), 0, ZMQ_POLLOUT, 0};
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

    if (zmq_send(this->socket_.get(), data, len, !timeout ? 0 : ZMQ_NOBLOCK) <= 0) {
        return Err(io::Error{io::ErrorKind::Other, "Error send"});
    }
    return Ok(std::monostate{});
}
Result<size_t, io::Error> ZmqChannel::stream_read(uint8_t *data, size_t len) {
    if (len == 0) {
        return Ok<size_t>(0);
    }
    if (this->msg_read_ == 0) {
        zmq_pollitem_t pollitem = {this->socket_.get(), 0, ZMQ_POLLIN, 0};
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
        int ret = zmq_msg_recv(&this->last_msg_, this->socket_.get(), ZMQ_NOBLOCK);
        if (ret <= 0) {
            return Err(io::Error{io::ErrorKind::Other, "Error receive"});
        }
    }

    size_t msg_len = zmq_msg_size(&this->last_msg_);
    const uint8_t *msg_data = static_cast<const uint8_t *>(zmq_msg_data(&this->last_msg_));
    size_t read_len = std::min(msg_len - this->msg_read_, len);
    memcpy(data, msg_data + this->msg_read_, read_len);
    this->msg_read_ += read_len;
    if (this->msg_read_ >= msg_len) {
        zmq_msg_close(&this->last_msg_);
        this->msg_read_ = 0;
    }
    return Ok(read_len);
}
