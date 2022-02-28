#pragma once

#include <functional>
#include <type_traits>

#include <core/assert.hpp>
#include <core/fmt.hpp>

template <typename T>
class Slice;

namespace slice_impl {

// Replace with std::span in C++20
template <typename T>
class BasicSlice {
private:
    T *ptr_ = nullptr;
    size_t size_ = 0;

public:
    BasicSlice() = default;
    BasicSlice(T *ptr, size_t size) : ptr_(ptr), size_(size) {}

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

    operator ::Slice<const T>() const {
        return ::Slice{ptr_, size_};
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


template <typename T, bool = std::is_trivial_v<T> && !std::is_const_v<T>>
class StreamSlice : public BasicSlice<T> {
public:
    using BasicSlice<T>::BasicSlice;
};

template <typename T>
class StreamSlice<T, true> :
    public BasicSlice<T>,
    public virtual ReadArrayExact<T>,
    public virtual ReadArrayInto<T> //
{
public:
    using BasicSlice<T>::BasicSlice;

    [[nodiscard]] bool read_array_exact(T *data, size_t len) override {
        if (len > this->size()) {
            return false;
        }
        memcpy(data, this->data(), sizeof(T) * len);
        assert_eq(this->skip_front(len), len);
        return true;
    }

    size_t read_array(T *data, size_t len) override {
        size_t read_len = std::min(this->size(), len);
        assert_true(read_array_exact(data, read_len));
        return read_len;
    }

    size_t read_array_into(WriteArray<T> &stream, std::optional<size_t> len) override {
        size_t write_len = stream.write_array(this->data(), len.value_or(this->size()));
        assert_eq(this->skip_front(write_len), write_len);
        return write_len;
    }
};

} // namespace slice_impl

template <typename T>
class Slice final : public slice_impl::StreamSlice<T> {
public:
    using slice_impl::StreamSlice<T>::StreamSlice;
};

template <typename T>
struct Display<Slice<T>, void> : public std::integral_constant<bool, is_display_v<T>> {};

template <typename T, typename = std::enable_if_t<is_display_v<Slice<T>>, void>>
std::ostream &operator<<(std::ostream &os, const Slice<T> &value) {
    auto it = value.begin();
    os << "[";
    if (it != value.end()) {
        os << *it;
        ++it;
    }
    for (; it != value.end(); ++it) {
        os << ", " << *it;
    }
    os << "]";
    return os;
}
