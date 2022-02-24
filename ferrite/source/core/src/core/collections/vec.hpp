#pragma once

#include <vector>
#include <cstring>
#include <type_traits>

#include <core/stream.hpp>
#include <core/io.hpp>

#include "slice.hpp"

template <typename T>
class __Vec : public std::vector<T> {
public:
    [[nodiscard]] Slice<T> slice() {
        return Slice<T>(this->data(), this->size());
    }
    [[nodiscard]] Slice<const T> slice() const {
        return Slice<T>(this->data(), this->size());
    }

    using std::vector<T>::vector;
};

template <typename T, bool = std::is_trivial_v<T>>
class _Vec : public __Vec<T> {
public:
    using __Vec<T>::__Vec;
};

template <typename T>
class _Vec<T, true> :
    public __Vec<T>,
    public virtual StreamWriteExact<T>,
    public virtual StreamReadFrom<T> //
{
public:
    using __Vec<T>::__Vec;

    [[nodiscard]] size_t stream_write(const T *data, size_t len) override {
        size_t size = this->size();
        this->resize(size + len);
        memcpy(this->data() + size, data, sizeof(T) * len);
        return len;
    }

    [[nodiscard]] bool stream_write_exact(const T *data, size_t len) override {
        assert_eq(this->stream_write(data, len), len);
        return true;
    }

    [[nodiscard]] size_t stream_read_from(StreamRead<T> &stream, std::optional<size_t> len_opt) override {
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
            size_t read_len = stream.stream_read(this->data() + size, len);
            this->resize(size + read_len);
            return read_len;
        } else {
            // Read infinitely until stream ends.
            size_t total = 0;
            for (;;) {
                size_t free = this->capacity() - this->size();
                if (free > 0) {
                    size_t res_len = stream_read_from(stream, free);
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


template <typename T>
class Vec final : public _Vec<T> {
public:
    using _Vec<T>::_Vec;
};

template <>
class Vec<uint8_t> final : public _Vec<uint8_t>, public virtual io::WriteExact, public virtual io::ReadFrom {
public:
    using _Vec<uint8_t>::_Vec;

    inline Result<size_t, io::Error> write(const uint8_t *data, size_t len) override {
        return Ok(this->stream_write(data, len));
    }

    inline Result<std::monostate, io::Error> write_exact(const uint8_t *data, size_t len) override {
        if (this->stream_write_exact(data, len)) {
            return Ok(std::monostate{});
        } else {
            return Err(io::Error{io::ErrorKind::UnexpectedEof});
        }
    }

    inline Result<size_t, io::Error> read_from(io::Read &stream, std::optional<size_t> len_opt) override {
        io::StreamReadWrapper wrapper{stream};
        size_t read_len = this->stream_read_from(wrapper, len_opt);
        if (wrapper.error.has_value()) {
            return Err(std::move(wrapper.error.value()));
        } else {
            return Ok(read_len);
        }
    }
};
