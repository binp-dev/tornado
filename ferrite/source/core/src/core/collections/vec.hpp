#pragma once

#include <vector>
#include <cstring>

#include <core/io.hpp>

#include "slice.hpp"

template <typename T>
class _Vec : public std::vector<T> {
public:
    [[nodiscard]] Slice<T> slice() {
        return Slice<T>(this->data(), this->size());
    }
    [[nodiscard]] Slice<const T> slice() const {
        return Slice<T>(this->data(), this->size());
    }

    using std::vector<T>::vector;
};

template <typename T>
class Vec final : public _Vec<T> {
public:
    using _Vec<T>::_Vec;
};

template <>
class Vec<uint8_t> final : public _Vec<uint8_t>, public virtual io::WriteExact, public virtual io::ReadFrom {
public:
    using _Vec<uint8_t>::_Vec;

    inline Result<std::monostate, io::Error> write_exact(const uint8_t *data, size_t len) override {
        size_t size = this->size();
        resize(size + len);
        memcpy(this->data() + size, data, len);
        return Ok(std::monostate{});
    }

    inline Result<size_t, io::Error> read_from(io::Read &stream, std::optional<size_t> len_opt) override {
        if (len_opt.has_value()) {
            size_t len = len_opt.value();
            size_t size = this->size();

            // Reserve enough space for new elements.
            size_t new_cap = std::max(this->capacity(), size_t(1));
            while (new_cap < size + len) {
                new_cap = new_cap * 2;
            }
            this->resize(new_cap);

            // Read from stream.
            auto res = stream.read(this->data() + size, len);
            size_t read_len = 0;
            if (res.is_ok()) {
                read_len = res.ok();
            }
            this->resize(size + read_len);
            return res;
        } else {
            // Read infinitely until stream ends.
            size_t total = 0;
            for (;;) {
                size_t free = this->capacity() - this->size();
                if (free > 0) {
                    auto res = read_from(stream, free);
                    if (res.is_err()) {
                        if (total == 0) {
                            return res;
                        } else {
                            return Ok(total);
                        }
                    }
                    total += res.ok();
                    if (res.ok() < free) {
                        return Ok(total);
                    }
                }
                this->reserve(std::max(this->capacity() * 2, size_t(1)));
            }
        }
    }
};
