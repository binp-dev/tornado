#pragma once

#include <mutex>

namespace core {

template <typename T>
class Mutex final {
public:
    class Guard final {
    private:
        std::lock_guard<std::mutex> guard_;
        T &value_ref_;

    public:
        Guard() = delete;
        Guard(std::mutex &mutex, T &value_ref) : guard_(mutex), value_ref_(value_ref) {}
        ~Guard() = default;

        Guard(const Guard &) = delete;
        Guard &operator=(const Guard &) = delete;
        Guard(Guard &&) = default;
        Guard &operator=(Guard &&) = default;

        T &operator*() {
            return value_ref_;
        }
        const T &operator*() const {
            return value_ref_;
        }
        T *operator->() {
            return &value_ref_;
        }
        const T *operator->() const {
            return &value_ref_;
        }
    };

private:
    mutable std::mutex mutex_;
    mutable T value_;

public:
    Mutex() = default;
    ~Mutex() = default;

    Mutex(T &&value) : value_(std::move(value)) {}
    Mutex(const T &value) : value_(value) {}

    Mutex(Mutex &&other) {
        value_ = std::move(other.value_);
    }
    Mutex &operator=(Mutex &&other) {
        value_ = std::move(other.value_);
        return *this;
    }

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    T replace(T &&value) {
        T tmp = std::move(value_);
        value_ = std::move(value);
        return std::move(tmp);
    }

    Guard lock() const {
        return Guard(mutex_, value_);
    }
};

} // namespace core
