#pragma once

#include <string>

#include "channel.hpp"

class ZmqChannel : public Channel {
public:
    ZmqChannel(const std::string &host) {

    }
    ~ZmqChannel() override {

    }

    void send(const uint8_t *bytes, size_t length) override {

    }
    size_t receive(uint8_t *bytes, size_t max_length) override {
        
    }
};
