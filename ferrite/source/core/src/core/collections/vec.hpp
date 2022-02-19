#pragma once

#include <vector>
#include <cstring>

#include <core/io.hpp>


struct VecStream final : public virtual io::WriteExact {
    std::vector<uint8_t> vector;

    inline Result<std::monostate, io::Error> write_exact(const uint8_t *data, size_t len) override {
        size_t size = vector.size();
        vector.resize(size + len);
        memcpy(vector.data() + size, data, len);
        return Ok(std::monostate{});
    }
};
