#pragma once

#include <span>
#include <functional>
#include <type_traits>

#include <core/assert.hpp>
#include <core/format.hpp>

namespace core {

template <typename T>
class Slice;

namespace _impl {

template <typename T>
class BasicSlice : public std::span<T> {
public:
    using std::span<T>::span;

private:
    void assign_this(const std::span<T> &span) {
        static_cast<std::span<T> &>(*this) = span;
    }

public:
    operator Slice<const T>() const {
        return Slice<const T>(*this);
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

    [[nodiscard]] bool read_array_exact(std::span<T> data) override {
        if (data.size() > this->size()) {
            return false;
        }
        memcpy(data.data(), this->data(), sizeof(T) * data.size());
        core_assert_eq(this->skip_front(data.size()), data.size());
        return true;
    }

    size_t read_array(std::span<T> data) override {
        size_t read_len = std::min(this->size(), data.size());
        core_assert(read_array_exact(data.subspan(0, read_len)));
        return read_len;
    }

    size_t read_array_into(WriteArray<T> &stream, std::optional<size_t> len) override {
        size_t write_len = stream.write_array(this->subspan(0, len.value_or(this->size())));
        core_assert_eq(this->skip_front(write_len), write_len);
        return write_len;
    }
};

} // namespace _impl

template <typename T>
class Slice final : public _impl::StreamSlice<T> {
public:
    using _impl::StreamSlice<T>::StreamSlice;
};

template <Printable T>
struct Print<std::span<T>> {
    static void print(std::ostream &os, const std::span<T> &value) {
        auto it = value.begin();
        os << "[";
        if (it != value.end()) {
            Print<T>::print(os, *it);
            ++it;
        }
        for (; it != value.end(); ++it) {
            os << ", ";
            Print<T>::print(os, *it);
        }
        os << "]";
    }
};

template <Printable T>
struct Print<Slice<T>> {
    static void print(std::ostream &os, const Slice<T> &value) {
        Print<std::span<T>>::print(os, value);
    }
};

} // namespace core
