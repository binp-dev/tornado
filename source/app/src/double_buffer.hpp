#pragma once

#include <atomic>

#include <core/mutex.hpp>
#include <core/collections/vec_deque.hpp>

template <typename T>
class DoubleBuffer final :
    public virtual core::WriteArrayExact<T>,
    public virtual core::ReadArrayInto<T> //
{
private:
    core::VecDeque<T> read_buffer_;
    core::Mutex<core::VecDeque<T>> write_buffer_;
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

    [[nodiscard]] bool write_array_exact(std::span<const T> data) override {
        auto write_buffer_guard = write_buffer_.lock();
        write_buffer_guard->clear();
        if (write_buffer_guard->write_array_exact(data)) {
            swapped_.store(false);
            return true;
        } else {
            return false;
        }
    }

    /// @note Calling this method will cause an infinite loop if DoubleBuffer is in cyclic mode and `stream` is infinite
    /// (e.g. `stream.write()` never returns zero).
    size_t read_array_into(core::WriteArray<T> &stream, std::optional<size_t> len_opt) override {
        size_t total_len = read_buffer_.read_array_into(stream, len_opt);
        while (!len_opt.has_value() || total_len < len_opt.value()) {
            swap();

            auto rem_opt = len_opt.has_value() ? //
                std::optional(len_opt.value() - total_len) :
                std::nullopt;

            size_t len = read_buffer_.read_array_into(stream, rem_opt);
            if (len == 0) {
                break;
            }
            total_len += len;
        }
        return total_len;
    }

    /// NOTE: Safe to call only from read side.
    void swap() {
        read_buffer_.clear();
        auto write_buffer_guard = write_buffer_.lock();
        if (!cyclic_.load()) {
            std::swap(read_buffer_, *write_buffer_guard);
        } else {
            write_buffer_guard->view().read_array_into(read_buffer_, std::nullopt);
            core_assert_eq(write_buffer_guard->size(), read_buffer_.size());
        }
        swapped_.store(true);
    }
};
