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
    public virtual core::io::StreamRead,
    public virtual core::io::StreamWrite,
    public virtual core::io::StreamWriteExact //
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
