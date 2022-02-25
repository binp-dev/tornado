#pragma once

#include <vector>
#include <cstring>
#include <type_traits>

#include <core/stream.hpp>

#include "slice.hpp"

namespace vec_impl {

template <typename T>
class BasicVec : public std::vector<T> {
public:
    using std::vector<T>::vector;

    [[nodiscard]] Slice<T> slice() {
        return Slice<T>(this->data(), this->size());
    }
    [[nodiscard]] Slice<const T> slice() const {
        return Slice<T>(this->data(), this->size());
    }
};


template <typename T, bool = std::is_trivial_v<T>>
class StreamVec : public BasicVec<T> {
public:
    using BasicVec<T>::BasicVec;
};

template <typename T>
class StreamVec<T, true> :
    public BasicVec<T>,
    public virtual WriteArrayExact<T>,
    public virtual ReadArrayFrom<T> //
{
public:
    using BasicVec<T>::BasicVec;

    [[nodiscard]] size_t write_array(const T *data, size_t len) override {
        size_t size = this->size();
        this->resize(size + len);
        memcpy(this->data() + size, data, sizeof(T) * len);
        return len;
    }

    [[nodiscard]] bool write_array_exact(const T *data, size_t len) override {
        assert_eq(this->write_array(data, len), len);
        return true;
    }

    [[nodiscard]] size_t read_array_from(ReadArray<T> &stream, std::optional<size_t> len_opt) override {
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
            size_t read_len = stream.read_array(this->data() + size, len);
            this->resize(size + read_len);
            return read_len;
        } else {
            // Read infinitely until stream ends.
            size_t total = 0;
            for (;;) {
                size_t free = this->capacity() - this->size();
                if (free > 0) {
                    size_t res_len = read_array_from(stream, free);
                    total += res_len;
                    if (res_len < free) {
                        return total;
                    }
                }
                this->reserve(std::max(this->capacity() * 2, size_t(1)));
            }
        }
    }
};

} // namespace vec_impl

template <typename T>
class Vec final : public vec_impl::StreamVec<T> {
public:
    using vec_impl::StreamVec<T>::StreamVec;
};