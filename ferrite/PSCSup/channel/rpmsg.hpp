#pragma once

#include <string>

#include "base.hpp"

class RpmsgChannel : public Channel {
public:
    RpmsgChannel(const std::string &dev) {

    }
    ~RpmsgChannel() override {

    }

    void send(const uint8_t *bytes, size_t length, msec timeout) override {

    }
    size_t receive(uint8_t *bytes, size_t max_length, msec timeout) override {
        
    }
};
