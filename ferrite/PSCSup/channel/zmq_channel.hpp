#pragma once

#include <string>
#include <memory>

#include <zmq.hpp>

#include "channel.hpp"


class ZmqChannel : public Channel {
private:
    std::string host;
    zmq::context_t context;
    zmq::socket_t socket;

public:
    ZmqChannel(const std::string &host) :
        host(host),
        context(1),
        socket(context, ZMQ_PAIR)
    {
        //std::cout << "ZmqChannel(host='" << host << "')" << std::endl;
        socket.connect(host);
    }
    ~ZmqChannel() override = default;

    void send(const uint8_t *bytes, size_t length, int timeout) override {
        zmq::pollitem_t pollitem = {(void *)socket, 0, ZMQ_POLLOUT};
        if (zmq::poll(&pollitem, 1, timeout) > 0) {
            if (socket.send(bytes, length, ZMQ_NOBLOCK) == 0) {
                throw Channel::IoError("Cannot send");
            }
        } else {
            throw Channel::TimedOut();
        }
    }
    size_t receive(uint8_t *bytes, size_t max_length, int timeout) override {
        zmq::pollitem_t pollitem = {(void *)socket, 0, ZMQ_POLLIN};
        if (zmq::poll(&pollitem, 1, timeout) > 0) {
            size_t ret = socket.recv(bytes, max_length, ZMQ_NOBLOCK);
            if (ret == 0) {
                throw Channel::IoError("Cannot receive");
            }
            return ret;
        } else {
            throw Channel::TimedOut();
        }
    }
};

#ifdef UNITTEST
#include <catch/catch.hpp>

#include <random>
#include <functional>

int try_random(int low, int high, int num_tries, std::function<void(int)> fn) {
    std::minstd_rand rng;
    rng.seed(0xdeadbeef);
    for (int i = 0; i < num_tries; ++i) {
        int value = (rng() % (high - low)) + low;
        try {
            fn(value);
            return value;
        } catch(...) {
            if (i >= num_tries - 1) {
                throw;
            }
        }
    }
    return -1;
}

void try_send(zmq::socket_t &socket, const void *data, size_t length) {
    zmq::pollitem_t pollitem = {(void *)socket, 0, ZMQ_POLLOUT};
    if (zmq::poll(&pollitem, 1, 10) > 0) {
        if (socket.send(data, length, ZMQ_NOBLOCK) == 0) {
            throw std::runtime_error("zmq send error");
        }
    } else {
        throw std::runtime_error("zmq send timeout");
    }
}

size_t try_recv(zmq::socket_t &socket, void *data, size_t max_length) {
    zmq::pollitem_t pollitem = {(void *)socket, 0, ZMQ_POLLIN};
    if (zmq::poll(&pollitem, 1, 10) > 0) {
        size_t ret = socket.recv(data, max_length, ZMQ_NOBLOCK);
        if (ret == 0) {
            throw std::runtime_error("zmq recv error");
        }
        return ret;
    } else {
        throw std::runtime_error("zmq recv timeout");
    }
}

TEST_CASE("ZeroMQ", "[zmq]") {
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
#endif // UNITTEST