#include <gtest/gtest.h>

//#include "zmq.hpp"
#include <core/result.hpp>
#include <core/panic.hpp>
#include <core/lazy_static.hpp>

#include <random>
#include <functional>

template <typename T>
std::optional<T> try_random(T low, T high, std::function<bool(T)> fn, size_t num_tries=16, uint32_t seed=0xDEADBEEF) {
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

/*
Result<std::monostate, std::string> try_send(zmq::socket_t &socket, const uint8_t *data, size_t length) {
    zmq::pollitem_t pollitem = {socket, 0, ZMQ_POLLOUT};
    if (zmq::poll(&pollitem, 1, 10) > 0) {
        if (socket.send(data, length, ZMQ_NOBLOCK) == 0) {
            return Err<std::string>("Zmq send error");
        }
    } else {
        return Err<std::string>("Zmq send timeout");
    }
}

size_t try_recv(zmq::socket_t &socket, void *data, size_t max_length) {
    zmq::pollitem_t pollitem = {socket, 0, ZMQ_POLLIN};
    if (zmq::poll(&pollitem, 1, 10) > 0) {
        size_t ret = socket.recv(data, max_length, ZMQ_NOBLOCK);
        if (ret == 0) {
            throw Err<std::string>("Zmq recv error");
        }
        return ret;
    } else {
        throw Err<std::string>("Zmq recv timeout");
    }
}
*/
/*
TEST(ZeroMQ, "[zmq]") {
    zmq::context_t ctx(1);

    SECTION("ZeroMQ PAIR socket") {
        zmq::socket_t one(ctx, ZMQ_PAIR);
        int port = try_random(5000, 6000, 10, [&one] (int port) {
            std::stringstream ss;
            ss << "tcp://127.0.0.1:" << port;
            one.bind(ss.str());
        });

        zmq::socket_t two(ctx, ZMQ_PAIR);
        std::stringstream ss;
        ss << "tcp://127.0.0.1:" << port;
        two.connect(ss.str());

        SECTION("one -> two") {
            const char *src = "Hello";
            try_send(one, src, 6);

            char dst[10];
            REQUIRE(try_recv(two, dst, 10) == 6);
            REQUIRE(strcmp(src, dst) == 0);
        }

        SECTION("two -> one") {
            const char *src = "World";
            try_send(two, src, 6);

            char dst[10];
            REQUIRE(try_recv(one, dst, 10) == 6);
            REQUIRE(strcmp(src, dst) == 0);
        }
    }

    SECTION("ZmqChannel wrapper") {
        zmq::socket_t one(ctx, ZMQ_PAIR);
        int port = try_random(5000, 6000, 10, [&one] (int port) {
            std::stringstream ss;
            ss << "tcp://127.0.0.1:" << port;
            one.bind(ss.str());
        });

        std::stringstream ss;
        ss << "tcp://127.0.0.1:" << port;
        ZmqChannel channel(ss.str());

        SECTION("Receive") {
            const char *src = "Hello";
            try_send(one, src, 6);

            char dst[10];
            REQUIRE(channel.receive((uint8_t *)dst, 10, 10) == 6);
            REQUIRE(strcmp(src, dst) == 0);
        }

        SECTION("Send") {
            const char *src = "World";
            channel.send((uint8_t *)src, 6, 10);

            char dst[10];
            REQUIRE(try_recv(one, dst, 10) == 6);
            REQUIRE(strcmp(src, dst) == 0);
        }
    }
}
*/
