#pragma once

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <chrono>
#include <iostream>

#include <core/io.hpp>
#include <core/result.hpp>

class Channel {
public:
    Channel() = default;
    virtual ~Channel() = default;

    Channel(Channel &&ch) = default;
    Channel &operator=(Channel &&ch) = default;

    Channel(const Channel &ch) = delete;
    Channel &operator=(const Channel &ch) = delete;

    virtual Result<std::monostate, io::Error> send(
        const uint8_t *bytes,
        size_t length,
        std::optional<std::chrono::milliseconds> timeout) = 0;

    virtual Result<size_t, io::Error> receive(
        uint8_t *bytes,
        size_t max_length,
        std::optional<std::chrono::milliseconds> timeout) = 0;
};
