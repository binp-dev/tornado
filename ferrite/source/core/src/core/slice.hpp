#pragma once

#include <core/assert.hpp>

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
