#pragma once

#include <string>

#include "channel.hpp"

class RpmsgChannel : public Channel {
public:
    RpmsgChannel(const std::string &dev) {

    }
    ~RpmsgChannel() override {

    }

    void send(const uint8_t *bytes, size_t length) override {

    }
    size_t receive(uint8_t *bytes, size_t max_length) override {
        
    }
};
