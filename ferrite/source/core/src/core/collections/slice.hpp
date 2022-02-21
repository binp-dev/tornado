#pragma once

#include <functional>

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

    [[nodiscard]] size_t size() const {
        return size_;
    }
    [[nodiscard]] bool is_empty() const {
        return size_ == 0;
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

    [[nodiscard]] std::optional<std::reference_wrapper<T>> pop_back() {
        if (is_empty()) {
            return std::nullopt;
        }
        size_ -= 1;
        return std::ref(ptr_[size_]);
    }
    [[nodiscard]] std::optional<std::reference_wrapper<T>> pop_front() {
        if (is_empty()) {
            return std::nullopt;
        }
        auto ret = std::ref(*ptr_);
        ptr_ += 1;
        size_ -= 1;
        return ret;
    }

    size_t skip_back(size_t count) {
        size_t skip = std::min(count, size_);
        size_ -= skip;
        return skip;
    }

    size_t skip_front(size_t count) {
        size_t skip = std::min(count, size_);
        ptr_ += skip;
        size_ -= skip;
        return skip;
    }

public:
    using iterator = T *;
    using const_iterator = const T *;

    [[nodiscard]] iterator begin() {
        return ptr_;
    }
    [[nodiscard]] iterator end() {
        return ptr_;
    }
    [[nodiscard]] const_iterator begin() const {
        return ptr_;
    }
    [[nodiscard]] const_iterator end() const {
        return ptr_;
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
