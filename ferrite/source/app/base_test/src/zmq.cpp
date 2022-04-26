#include <channel/zmq.hpp>

#include <random>
#include <functional>

#include <zmq.h>
#include <gtest/gtest.h>

#include <core/assert.hpp>
#include <core/result.hpp>

using namespace core;

inline constexpr auto TIMEOUT = std::chrono::milliseconds(100);

template <typename T>
std::optional<T> try_random(T low, T high, std::function<bool(T)> fn, size_t num_tries = 16, uint32_t seed = 0xDEADBEEF) {
    std::minstd_rand rng;
    rng.seed(seed);
    for (size_t i = 0; i < num_tries; ++i) {
        T value = (rng() % (high - low)) + low;
        if (fn(value)) {
            return std::optional(value);
        }
    }
    return std::nullopt;
}

Result<std::monostate, std::string> try_send(void *socket, const uint8_t *data, size_t length) {
    zmq_pollitem_t pollitem = {socket, 0, ZMQ_POLLOUT, 0};
    int ready = zmq_poll(&pollitem, 1, TIMEOUT.count());
    if (ready > 0) {
        if (zmq_send(socket, data, length, ZMQ_DONTWAIT) <= 0) {
            return Err<std::string>("Zmq send error");
        }
        return Ok(std::monostate{});
    } else if (ready == 0) {
        return Err<std::string>("Zmq send timeout");
    } else {
        return Err<std::string>("Zmq send error");
    }
}

Result<size_t, std::string> try_recv(void *socket, uint8_t *data, size_t max_length) {
    zmq_pollitem_t pollitem = {socket, 0, ZMQ_POLLIN, 0};
    int ready = zmq_poll(&pollitem, 1, TIMEOUT.count());
    if (ready > 0) {
        int ret = zmq_recv(socket, data, max_length, ZMQ_NOBLOCK);
        if (ret <= 0) {
            return Err<std::string>("Zmq recv error");
        }
        return Ok(size_t(ret));
    } else if (ready == 0) {
        return Err<std::string>("Zmq recv timeout");
    } else {
        return Err<std::string>("Zmq recv error");
    }
}


TEST(ZmqTest, sockets) {
    void *ctx = zmq_ctx_new();
    ASSERT_NE(ctx, nullptr);
    zmq::Context ctx_guard = zmq::guard_context(ctx);

    // one
    void *one = zmq_socket(ctx, ZMQ_PAIR);
    ASSERT_NE(one, nullptr);
    zmq::Socket one_guard = zmq::guard_socket(one);
    auto rp = try_random<uint16_t>(5000, 6000, [&one](uint16_t port) {
        std::string addr = "tcp://127.0.0.1:" + std::to_string(port);
        return zmq_bind(one, addr.c_str()) == 0;
    });
    ASSERT_TRUE(bool(rp));
    uint16_t port = *rp;

    // two
    void *two = zmq_socket(ctx, ZMQ_PAIR);
    ASSERT_NE(two, nullptr);
    zmq::Socket two_guard = zmq::guard_socket(two);
    std::string addr = "tcp://127.0.0.1:" + std::to_string(port);
    ASSERT_EQ(zmq_connect(two, addr.c_str()), 0);

    { // one -> two
        const char *src = "Hello";
        ASSERT_TRUE(try_send(one, (const uint8_t *)src, 6).is_ok());

        char dst[10];
        ASSERT_EQ(try_recv(two, (uint8_t *)dst, 10), Ok<size_t>(6));
        ASSERT_EQ(strcmp(src, dst), 0);
    }

    { // two -> one
        const char *src = "World";
        ASSERT_TRUE(try_send(two, (const uint8_t *)src, 6).is_ok());

        char dst[10];
        ASSERT_EQ(try_recv(one, (uint8_t *)dst, 10), Ok<size_t>(6));
        ASSERT_EQ(strcmp(src, dst), 0);
    }
}

TEST(ZmqTest, channel) {
    void *ctx = zmq_ctx_new();
    ASSERT_NE(ctx, nullptr);
    zmq::Context ctx_guard = zmq::guard_context(ctx);

    auto make_socket = [&]() -> std::pair<zmq::Socket, uint16_t> {
        void *socket = zmq_socket(ctx, ZMQ_PAIR);
        core_assert_ne(socket, nullptr);
        zmq::Socket socket_guard = zmq::guard_socket(socket);
        auto port = try_random<uint16_t>(5000, 6000, [&socket](uint16_t port) {
            std::string addr = "tcp://127.0.0.1:" + std::to_string(port);
            return zmq_bind(socket, addr.c_str()) == 0;
        });
        core_assert(port.has_value());
        return std::make_pair(std::move(socket_guard), port.value());
    };
    auto [send_socket, recv_port] = make_socket();
    auto [recv_socket, send_port] = make_socket();

    auto cr = ZmqChannel::create("127.0.0.1", send_port, recv_port);
    ASSERT_TRUE(cr.is_ok()) << cr.err().message;
    ZmqChannel channel = cr.unwrap();
    channel.timeout = TIMEOUT;

    { // Receive
        const char *src = "Hello";
        ASSERT_TRUE(try_send(send_socket.get(), (const uint8_t *)src, 6).is_ok());

        char dst[10];
        auto rr = channel.stream_read(std::span((uint8_t *)dst, 10));
        ASSERT_TRUE(rr.is_ok()) << rr.err().message;
        ASSERT_EQ(rr, Ok<size_t>(6)) << rr.ok();
        ASSERT_EQ(strcmp(src, dst), 0);
    }

    { // Send
        const char *src = "World";
        ASSERT_TRUE(channel.stream_write_exact(std::span((const uint8_t *)src, 6)).is_ok());

        char dst[10];
        ASSERT_EQ(try_recv(recv_socket.get(), (uint8_t *)dst, 10), Ok<size_t>(6));
        ASSERT_EQ(strcmp(src, dst), 0);
    }
}
