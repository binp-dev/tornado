#pragma once

#include <vector>
#include <optional>
#include <memory>

#include <core/mutex.hpp>
#include <core/collections/vec_deque.hpp>

template <typename T>
class BufferQueue {
private:
    using Half = Mutex<VecDeque<std::vector<T>>>;

public:
    class Guard final {
    private:
        std::vector<T> buffer_;
        std::weak_ptr<Half> destination_;

    public:
        void cleanup() {
            auto dst = destination_.lock();
            if (dst != nullptr) {
                dst->lock()->push_back(std::move(buffer_));
            }
        }

        Guard(std::vector<T> &&buf, std::weak_ptr<Half> dst) : buffer_(std::move(buf)), destination_(dst) {}
        ~Guard() {
            cleanup();
        }

        Guard(Guard &other) : buffer_(std::move(other.buffer_)), destination_(other.destination_) {
            other.destination_.reset();
        }
        Guard &operator=(Guard &other) {
            cleanup();

            buffer_ = std::move(other.buffer_);
            destination_ = std::move(other.destination_);
            other.destination_.reset();

            return *this;
        }

        Guard(const Guard &) = delete;
        Guard &operator=(const Guard &) = delete;

        T &operator*() {
            return buffer_;
        }
        const T &operator*() const {
            return buffer_;
        }
        T *operator->() {
            return &buffer_;
        }
        const T *operator->() const {
            return &buffer_;
        }
    };

private:
    std::shared_ptr<Half> vacant_;
    std::shared_ptr<Half> occupied_;

public:
    explicit BufferQueue(size_t buffers_count, size_t max_buffer_size) {
        vacant_.reserve(buffers_count);
        occupied_.reserve(buffers_count);
        for (size_t i = 0; i < max_size; ++i) {
            std::vector<T> buffer;
            buffer.reserve(max_buffer_size);
            vacant_.push_back(std::move(buffer));
        }
    }
};
