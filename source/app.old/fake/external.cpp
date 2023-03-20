#include <external.hpp>

#include <channel/zmq.hpp>

size_t max_message_length() {
    return 1024;
}

std::unique_ptr<Channel> make_device_channel() {
    return std::make_unique<ZmqChannel>(std::move(ZmqChannel::create("127.0.0.1", 8321, 8322).unwrap()));
}
