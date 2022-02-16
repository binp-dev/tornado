#include <external.hpp>

#include <channel/zmq.hpp>

std::unique_ptr<Channel> make_device_channel() {
    return std::make_unique<ZmqChannel>(std::move(ZmqChannel::create("tcp://127.0.0.1:8321").unwrap()));
}
