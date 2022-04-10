#pragma once

#include <span>
#include <functional>
#include <type_traits>

#include <core/assert.hpp>
#include <core/fmt.hpp>

template <typename T>
class Slice;

namespace slice_impl {

template <typename T>
class BasicSlice : public std::span<T> {
public:
    using std::span<T>::span;

private:
    void assign_this(const std::span<T> &span) {
        static_cast<std::span<T> &>(*this) = span;
    }

public:
    operator ::Slice<const T>() const {
        return ::Slice<const T>(*this);
    }

    [[nodiscard]] std::optional<std::reference_wrapper<T>> pop_back() {
        if (this->empty()) {
            return std::nullopt;
        }
        auto ret = std::ref(this->back());
        assign_this(this->subspan(0, this->size() - 1));
        return ret;
    }
    [[nodiscard]] std::optional<std::reference_wrapper<T>> pop_front() {
        if (this->empty()) {
            return std::nullopt;
        }
        auto ret = std::ref(this->front());
        assign_this(this->subspan(1, this->size() - 1));
        return ret;
    }

    size_t skip_back(size_t count) {
        size_t skip = std::min(count, this->size());
        assign_this(this->subspan(0, this->size() - skip));
        return skip;
    }

    size_t skip_front(size_t count) {
        size_t skip = std::min(count, this->size());
        assign_this(this->subspan(skip, this->size() - skip));
        return skip;
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
