#pragma once

#include <core/mutex.hpp>
#include <core/collections/vec_deque.hpp>

template <typename T>
class DoubleBuffer final : public virtual ReadArrayInto<T> {
private:
    VecDeque<T> read_buffer_;
    Mutex<VecDeque<T>> write_buffer_;

public:
    [[nodiscard]] VecDeque<T> &read_buffer() {
        return read_buffer_;
    }
    [[nodiscard]] typename Mutex<VecDeque<T>>::Guard write_buffer() {
        return write_buffer_.lock();
    }

    [[nodiscard]] bool empty() const {
        return read_buffer_.empty() && write_buffer_.lock()->empty();
    }

    /// NOTE: Safe to call only from read side.
    void swap() {
        read_buffer_.clear();
        std::swap(read_buffer_, *(write_buffer_.lock()));
    }

    size_t read_array_into(WriteArray<T> &stream, std::optional<size_t> len_opt) {
        size_t first_len = read_buffer().read_array_into(stream, len_opt);
        size_t second_len = 0;
        if (!len_opt.has_value() || first_len < len_opt.value()) {
            swap();
            auto len_opt_2 = len_opt.has_value() ? //
                std::optional(len_opt.value() - first_len) :
                std::nullopt;
            second_len = read_buffer().read_array_into(stream, len_opt_2);
        }
        return first_len + second_len;
    }
};
