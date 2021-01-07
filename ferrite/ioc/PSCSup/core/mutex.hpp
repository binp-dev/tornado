#pragma once

#include <mutex>

template <typename T>
class Mutex final {
public:
    class Guard final {
    private:
        std::lock_guard<std::mutex> guard_;
        T &value_ref_;

    public:
        Guard() = delete;
        Guard(std::mutex &mutex, T &value_ref) :
            guard_(mutex), value_ref_(value_ref)
        {}
        ~Guard() = default;

        Guard(const Guard &) = delete;
        Guard &operator=(const Guard &) = delete;
        Guard(Guard &&) = delete;
        Guard &operator=(Guard &&) = delete;

        T &operator*() { return value_ref_; }
        const T &operator*() const { return value_ref_; }
        T *operator->() { return &value_ref_; }
        const T *operator->() const { return &value_ref_; }
    };

private:
    mutable std::mutex mutex_;
    mutable T value_;

public:
    Mutex() = default;
    Mutex(T &&value) : value_(std::move(value)) {}
    Mutex(const T &value) : value_(value) {}
    ~Mutex() = default;

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;
    Mutex(Mutex &&) = delete;
    Mutex &operator=(Mutex &&) = delete;

    T replace(T &&value) {
        T tmp = std::move(value_);
        value_ = std::move(value);
        return std::move(tmp);
    }

    Guard lock() const {
        return Guard(mutex_, value_);
    }
};
