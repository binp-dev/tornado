#pragma once

#include <vector>
#include <cstring>

#include <core/io.hpp>


template <typename T>
struct Vec final : public std::vector<T> {
    using std::vector<T>::vector;
};

template <>
struct Vec<uint8_t> final : public std::vector<uint8_t>, public virtual io::WriteExact {
    using std::vector<uint8_t>::vector;

    inline Result<std::monostate, io::Error> write_exact(const uint8_t *data, size_t len) override {
        size_t size = this->size();
        resize(size + len);
        memcpy(this->data() + size, data, len);
        return Ok(std::monostate{});
    }
};
