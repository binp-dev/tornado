#pragma once

#include <cstdlib>
#include <cstdint>

class Channel {
public:
    Channel() = default;
    Channel(const Channel &ch) = delete;
    Channel &operator=(const Channel &ch) = delete;
    virtual ~Channel() = default;

    virtual void send(const uint8_t *bytes, size_t length) = 0;
    virtual size_t receive(uint8_t *bytes, size_t max_length) = 0;
};
