#pragma once

#include <functional>

#include <core/assert.hpp>
#include <core/io.hpp>

template <typename T>
class Slice;

// Replace with std::span in C++20
template <typename T>
class _Slice {
private:
    T *ptr_ = nullptr;
    size_t size_ = 0;

public:
    _Slice() = default;
    _Slice(T *ptr, size_t size) : ptr_(ptr), size_(size) {}

    T *data() {
        return ptr_;
    }
    const T *data() const {
        return ptr_;
    }

    [[nodiscard]] size_t size() const {
        return size_;
    }
    [[nodiscard]] bool empty() const {
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
        if (empty()) {
            return std::nullopt;
        }
        size_ -= 1;
        return std::ref(ptr_[size_]);
    }
    [[nodiscard]] std::optional<std::reference_wrapper<T>> pop_front() {
        if (empty()) {
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

template <typename T>
class Slice final : public _Slice<T> {
public:
    using _Slice<T>::_Slice;
};

template <>
class Slice<uint8_t> final : public _Slice<uint8_t>, public virtual io::ReadExact, public virtual io::WriteInto {
public:
    using _Slice<uint8_t>::_Slice;

    inline Result<std::monostate, io::Error> read_exact(uint8_t *data, size_t len) override {
        if (len > this->size()) {
            return Err(io::Error{io::ErrorKind::UnexpectedEof});
        }
        memcpy(data, this->data(), len);
        this->skip(len);
        return Ok(std::monostate{});
    }

    inline Result<size_t, io::Error> read(uint8_t *data, size_t len) override {
        size_t read_len = std::min(this->size(), len);
        assert_true(read_exact(data, read_len).is_ok());
        return Ok(read_len);
    }

    inline Result<size_t, io::Error> write_into(io::Write &stream, std::optional<size_t> len) override {
        size_t write_len = len.value_or(this->size());
        auto res = stream.write(this->data(), write_len);
        if (res.is_ok()) {
            this->skip_front(res.ok());
        }
        return res;
    }
};
