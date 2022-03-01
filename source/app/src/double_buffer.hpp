#pragma once

#include <atomic>

#include <core/mutex.hpp>
#include <core/collections/vec_deque.hpp>

template <typename T>
class DoubleBuffer final : public virtual WriteArrayExact<T>, public virtual ReadArrayInto<T> {
private:
    VecDeque<T> read_buffer_;
    Mutex<VecDeque<T>> write_buffer_;
    std::atomic<bool> cyclic_{false};
    std::atomic<bool> swapped_{false};

public:
    void reserve(size_t size) {
        read_buffer_.reserve(size);
        write_buffer_.lock()->reserve(size);
    }

    [[nodiscard]] bool cyclic() const {
        return cyclic_.load();
    }
    void set_cyclic(bool enabled) {
        return cyclic_.store(enabled);
    }

    [[nodiscard]] bool write_ready() const {
        return swapped_.load();
    }

    [[nodiscard]] bool write_array_exact(const T *data, size_t len) override {
        auto write_buffer_guard = write_buffer_.lock();
        write_buffer_guard->clear();
        if (write_buffer_guard->write_array_exact(data, len)) {
            swapped_.store(false);
            return true;
        } else {
            return false;
        }
    }

    size_t read_array_into(WriteArray<T> &stream, std::optional<size_t> len_opt) override {
        size_t first_len = read_buffer_.read_array_into(stream, len_opt);
        size_t second_len = 0;
        if (!len_opt.has_value() || first_len < len_opt.value()) {
            swap();
            auto len_opt_2 = len_opt.has_value() ? //
                std::optional(len_opt.value() - first_len) :
                std::nullopt;
            second_len = read_buffer_.read_array_into(stream, len_opt_2);
        }
        return first_len + second_len;
    }

    /// NOTE: Safe to call only from read side.
    void swap() {
        read_buffer_.clear();
        auto write_buffer_guard = write_buffer_.lock();
        if (!cyclic_.load()) {
            std::swap(read_buffer_, *write_buffer_guard);
        } else {
            write_buffer_guard->view().read_array_into(read_buffer_, std::nullopt);
            assert_eq(write_buffer_guard->size(), read_buffer_.size());
        }
        swapped_.store(true);
    }
};
