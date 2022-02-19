#pragma once

#include <core/assert.hpp>
#include <core/io.hpp>

template <typename T>
class Slice final {
private:
    T *ptr_ = nullptr;
    size_t size_ = 0;

public:
    Slice() = default;
    Slice(T *ptr, size_t size) : ptr_(ptr), size_(size) {}

    T *data() {
        return ptr_;
    }
    const T *data() const {
        return ptr_;
    }

    size_t size() const {
        return size_;
    }
    void skip(size_t count) {
        assert_true(count <= size_);
        size_ -= count;
        ptr_ += count;
    }

    T &operator[](size_t i) {
        assert_true(i < size_);
        return ptr_[i];
    }
    const T &operator[](size_t i) const {
        assert_true(i < size_);
        return ptr_[i];
    }

    operator Slice<const T>() const {
        return Slice{ptr_, size_};
    }
};

class SliceStream final : public virtual io::ReadExact {
    Slice<uint8_t> slice;

    inline Result<std::monostate, io::Error> read_exact(uint8_t *data, size_t len) override {
        if (len > slice.size()) {
            return Err(io::Error{io::ErrorKind::UnexpectedEof});
        }
        memcpy(data, slice.data(), len);
        slice.skip(len);
        return Ok(std::monostate{});
    }

    inline Result<size_t, io::Error> read(uint8_t *data, size_t len) override {
        size_t read_len = std::min(slice.size(), len);
        assert_true(read_exact(data, read_len).is_ok());
        return Ok(read_len);
    }
};
