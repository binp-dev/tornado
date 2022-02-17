#include "zmq.hpp"

#include <core/assert.hpp>


namespace zmq_helper {

void ContextDestroyer::operator()(void *context) const {
    assert_eq(zmq_ctx_destroy(context), 0);
}
void SocketCloser::operator()(void *socket) const {
    assert_eq(zmq_close(socket), 0);
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

    // std::cout << "ZmqChannel(host='" << host << "')" << std::endl;
    if (zmq_connect(socket.get(), host.c_str()) != 0) {
        return Err(io::Error{io::ErrorKind::Other, "Error connecting ZMQ socket"});
    }

    return Ok(ZmqChannel(host, std::move(context), std::move(socket)));
}

Result<std::monostate, io::Error> ZmqChannel::send(
    const uint8_t *bytes,
    size_t length,
    std::optional<std::chrono::milliseconds> timeout //
) {
    zmq_pollitem_t pollitem = {this->socket_.get(), 0, ZMQ_POLLOUT, 0};
    int count = zmq_poll(&pollitem, 1, timeout.has_value() ? zmq_helper::duration_to_microseconds(timeout.value()) : -1);
    if (count > 0) {
        if (!(pollitem.revents & ZMQ_POLLOUT)) {
            return Err(io::Error{io::ErrorKind::Other, "Poll bad event"});
        }
    } else if (count == 0) {
        return Err(io::Error{io::ErrorKind::TimedOut});
    } else {
        return Err(io::Error{io::ErrorKind::Other, "Poll error"});
    }

    if (zmq_send(this->socket_.get(), bytes, length, !timeout ? 0 : ZMQ_NOBLOCK) <= 0) {
        return Err(io::Error{io::ErrorKind::Other, "Error send"});
    }
    return Ok(std::monostate{});
}
Result<size_t, io::Error> ZmqChannel::receive(
    uint8_t *bytes,
    size_t max_length,
    std::optional<std::chrono::milliseconds> timeout //
) {
    zmq_pollitem_t pollitem = {this->socket_.get(), 0, ZMQ_POLLIN, 0};
    int count = zmq_poll(&pollitem, 1, timeout.has_value() ? zmq_helper::duration_to_microseconds(timeout.value()) : -1);
    if (count > 0) {
        if (!(pollitem.revents & ZMQ_POLLIN)) {
            return Err(io::Error{io::ErrorKind::Other, "Poll bad event"});
        }
    } else if (count == 0) {
        return Err(io::Error{io::ErrorKind::TimedOut});
    } else {
        return Err(io::Error{io::ErrorKind::Other, "Poll error"});
    }

    int ret = zmq_recv(this->socket_.get(), bytes, max_length, ZMQ_NOBLOCK);
    if (ret <= 0) {
        return Err(io::Error{io::ErrorKind::Other, "Error receive"});
    }
    return Ok<size_t>(ret);
}
