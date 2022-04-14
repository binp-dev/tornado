#pragma once

#include <vector>
#include <cstring>
#include <type_traits>
#include <iostream>

#include <core/stream.hpp>
#include <core/format.hpp>

#include "slice.hpp"

namespace core {

namespace _impl {

template <typename T>
class BasicVec : public std::vector<T> {
public:
    using std::vector<T>::vector;

    [[nodiscard]] Slice<T> slice() {
        return Slice<T>(this->data(), this->size());
    }
    [[nodiscard]] Slice<const T> slice() const {
        return Slice<const T>(this->data(), this->size());
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
    public virtual WriteArrayFrom<T> //
{
public:
    using BasicVec<T>::BasicVec;

    [[nodiscard]] size_t write_array(std::span<const T> data) override {
        size_t size = this->size();
        this->resize(size + data.size());
        memcpy(this->data() + size, data.data(), sizeof(T) * data.size());
        return data.size();
    }

    [[nodiscard]] bool write_array_exact(std::span<const T> data) override {
        core_assert_eq(this->write_array(data), data.size());
        return true;
    }

    size_t write_array_from(ReadArray<T> &stream, std::optional<size_t> len_opt) override {
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
            size_t read_len = stream.read_array(std::span(this->data() + size, len));
            this->resize(size + read_len);
            return read_len;
        } else {
            // Read infinitely until stream ends.
            size_t total = 0;
            for (;;) {
                size_t free = this->capacity() - this->size();
                if (free > 0) {
                    size_t res_len = write_array_from(stream, free);
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

} // namespace _impl

template <typename T>
class Vec final : public _impl::StreamVec<T> {
public:
    using _impl::StreamVec<T>::StreamVec;
};

template <Printable T>
struct Print<std::vector<T>> {
    static void print(std::ostream &os, const std::vector<T> &value) {
        Print<std::span<const T>>::print(os, std::span(value));
    }
};

template <Printable T>
struct Print<Vec<T>> {
    static void print(std::ostream &os, const Vec<T> &value) {
        Print<std::vector<T>>::print(os, value);
    }
};

} // namespace core
