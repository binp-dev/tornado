#pragma once

#include <functional>

#include <core/assert.hpp>
#include <core/io.hpp>

template <typename T>
class Slice;

// Replace with std::span in C++20
template <typename T>
class __Slice {
private:
    T *ptr_ = nullptr;
    size_t size_ = 0;

public:
    __Slice() = default;
    __Slice(T *ptr, size_t size) : ptr_(ptr), size_(size) {}

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
        return ptr_ + size_;
    }
    [[nodiscard]] const_iterator begin() const {
        return ptr_;
    }
    [[nodiscard]] const_iterator end() const {
        return ptr_ + size_;
    }
};

template <typename T, bool = std::is_trivial_v<T>>
class _Slice : public __Slice<T> {
public:
    using __Slice<T>::__Slice;
};

template <typename T>
class _Slice<T, true> :
    public __Slice<T>,
    public virtual StreamReadExact<T>,
    public virtual StreamWriteInto<T> //
{
public:
    using __Slice<T>::__Slice;

    [[nodiscard]] bool stream_read_exact(T *data, size_t len) override {
        if (len > this->size()) {
            return false;
        }
        memcpy(data, this->data(), sizeof(T) * len);
        assert_eq(this->skip_front(len), len);
        return true;
    }

    size_t stream_read(T *data, size_t len) override {
        size_t read_len = std::min(this->size(), len);
        assert_true(stream_read_exact(data, read_len));
        return read_len;
    }

    size_t stream_write_into(StreamWrite<T> &stream, std::optional<size_t> len) override {
        size_t write_len = stream.stream_write(this->data(), len.value_or(this->size()));
        assert_eq(this->skip_front(write_len), write_len);
        return write_len;
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
        if (this->stream_read_exact(data, len)) {
            return Ok(std::monostate{});
        } else {
            return Err(io::Error{io::ErrorKind::UnexpectedEof});
        }
    }

    inline Result<size_t, io::Error> read(uint8_t *data, size_t len) override {
        return Ok(this->stream_read(data, len));
    }

    inline Result<size_t, io::Error> write_into(io::Write &stream, std::optional<size_t> len) override {
        io::StreamWriteWrapper wrapper{stream};
        size_t write_len = this->stream_write_into(wrapper, len);
        if (wrapper.error.has_value()) {
            return Err(std::move(wrapper.error.value()));
        } else {
            return Ok(write_len);
        }
    }
};
