#pragma once

#include <core/mutex.hpp>
#include <core/collections/vec_deque.hpp>

template <typename T>
class DoubleBuffer final : public virtual ReadArray<T>, public virtual ReadArrayInto<T> {
public:
    using BufferGuard = typename Mutex<VecDeque<T>>::Guard;

private:
    VecDeque<T> read_buffer_;
    Mutex<VecDeque<T>> write_buffer_;

public:
    [[nodiscard]] VecDeque<T> read_buffer() {
        return read_buffer_;
    }
    [[nodiscard]] BufferGuard write_buffer() {
        return write_buffer_.lock();
    }
    /// NOTE: Safe to call only from read side.
    void swap() {
        read_buffer_.clear();
        std::swap(read_buffer_, *(write_buffer_.lock()));
    }

    [[nodiscard]] size_t read_array(T *data, size_t len) {
        size_t first_len = read_buffer().read_array(data, len);
        size_t second_len = 0;
        if (first_len < len) {
            swap();
            second_len = read_buffer().read_array(data, len - first_len);
        }
        return first_len + second_len;
    }

    size_t read_array_into(WriteArray<T> &stream, std::optional<size_t> len) {
        size_t first_len = read_buffer().read_array_into(stream, len);
        size_t second_len = 0;
        if (!len.has_value() || first_len < len.value()) {
            swap();
            second_len = read_buffer().read_array_into(
                stream,
                len.has_value() ? std::optional(len.value() - first_len) : std::nullopt);
        }
        return first_len + second_len;
    }
};
