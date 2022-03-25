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

class Channel :
    public virtual io::StreamRead,
    public virtual io::StreamWrite,
    public virtual io::StreamWriteExact //
{
public:
    std::optional<std::chrono::milliseconds> timeout = std::nullopt;

public:
    Channel() = default;
    virtual ~Channel() = default;

    Channel(Channel &&ch) = default;
    Channel &operator=(Channel &&ch) = default;

    Channel(const Channel &ch) = delete;
    Channel &operator=(const Channel &ch) = delete;
};
